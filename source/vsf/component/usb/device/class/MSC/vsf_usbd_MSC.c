/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*============================ INCLUDES ======================================*/

#include "component/usb/vsf_usb_cfg.h"

#if VSF_USE_USB_DEVICE == ENABLED && VSF_USE_USB_DEVICE_MSC == ENABLED

#define VSF_USBD_INHERIT
#define VSF_USBD_MSC_IMPLEMENT
// TODO: use dedicated include
#include "vsf.h"

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

enum {
    VSF_EVT_EXECUTE = VSF_EVT_USER + 0,
};

/*============================ PROTOTYPES ====================================*/

static vsf_err_t vk_usbd_msc_request_prepare(vk_usbd_dev_t *dev, vk_usbd_ifs_t *ifs);
static vsf_err_t vk_usbd_msc_class_init(vk_usbd_dev_t *dev, vk_usbd_ifs_t *ifs);

/*============================ GLOBAL VARIABLES ==============================*/

const vk_usbd_class_op_t vk_usbd_msc_class = {
    .request_prepare = vk_usbd_msc_request_prepare,
    .init = vk_usbd_msc_class_init,
};

/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

static void vk_usbd_msc_send_csw(void *p);
static void vk_usbd_msc_on_cbw(void *p);
static void vk_usbd_msc_on_idle(void *p);

/*============================ IMPLEMENTATION ================================*/

static void vk_usbd_msc_error(vk_usbd_msc_t *msc, uint_fast8_t error)
{
    usb_msc_cbw_t *cbw = &msc->ctx.cbw;
    bool is_in = (cbw->bmCBWFlags & USB_DIR_MASK) == USB_DIR_IN;

    msc->ctx.csw.dCSWStatus = error;
    if (is_in) {
        if (cbw->dCBWDataTransferLength > 0) {
            vk_usbd_trans_t *trans = &msc->ep_stream.use_as__vk_usbd_trans_t;
            trans->ep = msc->ep_in;
            trans->pchBuffer = NULL;
            trans->nSize = 0;
            trans->on_finish = vk_usbd_msc_send_csw;
            trans->param = msc;
            vk_usbd_ep_send(msc->dev, trans);
            return;
        }
    } else {
        vk_usbd_ep_stall(msc->dev, msc->ep_out);
    }
    vk_usbd_msc_send_csw(msc);
}

static void vk_usbd_msc_send_csw(void *p)
{
    vk_usbd_msc_t *msc = p;
    usb_msc_csw_t *csw = &msc->ctx.csw;
    vk_usbd_trans_t *trans = &msc->ep_stream.use_as__vk_usbd_trans_t;

    // TODO: fix csw->dCSWDataResidue
    csw->dCSWSignature = cpu_to_le32(USB_MSC_CSW_SIGNATURE);
    trans->ep = msc->ep_in;
    trans->pchBuffer = (uint8_t *)&msc->ctx.csw;
    trans->nSize = sizeof(msc->ctx.csw);
    trans->on_finish = vk_usbd_msc_on_idle;
    trans->param = msc;
    vk_usbd_ep_send(msc->dev, trans);
}

static void vk_usbd_msc_on_data_out(void *p)
{
    vsf_eda_post_evt(&((vk_usbd_msc_t *)p)->eda, VSF_EVT_EXECUTE);
}

static void vk_usbd_msc_on_data_in(void *p)
{
    vk_usbd_msc_t *msc = p;
    msc->ctx.csw.dCSWStatus = USB_MSC_CSW_OK;
    vk_usbd_msc_send_csw(msc);
}

static void vk_usbd_msc_on_cbw(void *p)
{
    vk_usbd_msc_t *msc = p;
    usb_msc_cbw_t *cbw = &msc->ctx.cbw;
    vk_usbd_trans_t *trans = &msc->ep_stream.use_as__vk_usbd_trans_t;

    if (    (trans->nSize > 0)
        ||  (cbw->dCBWSignature != cpu_to_le32(USB_MSC_CBW_SIGNATURE))
        ||  (cbw->bCBWCBLength < 1) || (cbw->bCBWCBLength > 16)) {
        vk_usbd_msc_error(msc, USB_MSC_CSW_PHASE_ERROR);
        return;
    }
    if (cbw->bCBWLUN > msc->max_lun) {
        vk_usbd_msc_error(msc, USB_MSC_CSW_FAIL);
        return;
    }

    if (    vk_scsi_prepare_buffer(msc->scsi, msc->ctx.cbw.CBWCB, &trans->use_as__vsf_mem_t)
        &&  ((cbw->bmCBWFlags & USB_DIR_MASK) == USB_DIR_OUT)
        &&  (cbw->dCBWDataTransferLength > 0)) {

        trans->ep = msc->ep_out;
        trans->on_finish = vk_usbd_msc_on_data_out;
        trans->param = msc;
        vk_usbd_ep_recv(msc->dev, trans);
    } else {
        vsf_eda_post_evt(&msc->eda, VSF_EVT_EXECUTE);
    }
}

static void vk_usbd_msc_on_idle(void *p)
{
    vk_usbd_msc_t *msc = p;
    vk_usbd_trans_t *trans = &msc->ep_stream.use_as__vk_usbd_trans_t;

    trans->ep = msc->ep_out;
    trans->pchBuffer = (uint8_t *)&msc->ctx.cbw;
    trans->nSize = sizeof(msc->ctx.cbw);
    trans->on_finish = vk_usbd_msc_on_cbw;
    trans->param = msc;
    vk_usbd_ep_recv(msc->dev, trans);
}

static void vk_usbd_msc_evthandler(vsf_eda_t *eda, vsf_evt_t evt)
{
    vk_usbd_msc_t *msc = container_of(eda, vk_usbd_msc_t, eda);
    usb_msc_cbw_t *cbw = &msc->ctx.cbw;
    bool is_in = (cbw->bmCBWFlags & USB_DIR_MASK) == USB_DIR_IN;
    vk_usbd_trans_t *trans = &msc->ep_stream.use_as__vk_usbd_trans_t;
    vsf_err_t errcode;
    uint32_t reply_len;

    switch (evt) {
    case VSF_EVT_INIT:
        msc->is_inited = false;
        vk_scsi_init(msc->scsi);
        break;
    case VSF_EVT_RETURN:
        errcode = vk_scsi_get_errcode(msc->scsi, &reply_len);
        if (!msc->is_inited) {
            if (errcode) {
                // fail to initialize scsi
                VSF_USB_ASSERT(false);
                return;
            }
            msc->is_inited = true;
            vk_usbd_msc_on_idle(msc);
        } else {
            if (errcode) {
                vk_usbd_msc_error(msc, USB_MSC_CSW_FAIL);
                break;
            }

            if (is_in && (cbw->dCBWDataTransferLength > 0)) {
                if (!msc->is_stream) {
                    trans->nSize = reply_len;
                    trans->ep = msc->ep_in;
                    trans->on_finish = vk_usbd_msc_on_data_in;
                    trans->param = msc;
                    vk_usbd_ep_send(msc->dev, trans);
                }
            } else {
                vk_usbd_msc_send_csw(msc);
            }
        }
        break;
    case VSF_EVT_EXECUTE:
        if (cbw->dCBWDataTransferLength > 0) {
            if (trans->pchBuffer != NULL) {
                msc->is_stream = false;
                vk_scsi_execute(msc->scsi, cbw->CBWCB, &trans->use_as__vsf_mem_t);
            } else if (msc->stream != NULL) {
                msc->is_stream = true;
                msc->ep_stream.stream = msc->stream;
                vsf_stream_init(msc->stream);
                if (is_in) {
                    msc->ep_stream.ep = msc->ep_in;
                    msc->ep_stream.callback.on_finish = vk_usbd_msc_on_data_in;
                    vk_usbd_ep_send_stream(&msc->ep_stream, cbw->dCBWDataTransferLength);
                } else {
                    msc->ep_stream.ep = msc->ep_out;
                    msc->ep_stream.callback.on_finish = NULL;
                    vk_usbd_ep_recv_stream(&msc->ep_stream, cbw->dCBWDataTransferLength);
                }
                vk_scsi_execute_stream(msc->scsi, cbw->CBWCB, msc->stream);
            } else {
                // how to get the buffer?
                VSF_USB_ASSERT(false);
            }
        } else {
            vk_scsi_execute(msc->scsi, cbw->CBWCB, NULL);
        }
        break;
    }
}

static vsf_err_t vk_usbd_msc_class_init(vk_usbd_dev_t *dev, vk_usbd_ifs_t *ifs)
{
    vk_usbd_msc_t *msc = ifs->class_param;

    msc->dev = dev;
    memset(&msc->ep_stream, 0, sizeof(msc->ep_stream));
    msc->ep_stream.dev = dev;
    msc->ep_stream.zlp = false;
    msc->ep_stream.callback.param = msc;

    vsf_eda_set_evthandler(&msc->eda, vk_usbd_msc_evthandler);
    return vsf_eda_init(&msc->eda, VSF_USBD_CFG_EDA_PRIORITY, false);
}

static vsf_err_t vk_usbd_msc_request_prepare(vk_usbd_dev_t *dev, vk_usbd_ifs_t *ifs)
{
    vk_usbd_msc_t *msc = ifs->class_param;
    vk_usbd_ctrl_handler_t *ctrl_handler = &dev->ctrl_handler;
    struct usb_ctrlrequest_t *request = &ctrl_handler->request;
    uint8_t *buffer = NULL;
    uint_fast32_t size = 0;

    switch (request->bRequest) {
    case USB_MSC_REQ_GET_MAX_LUN:
        if ((request->wLength != 1) || (request->wValue != 0)) {
            return VSF_ERR_FAIL;
        }
        // TODO: process alignment
        buffer = (uint8_t *)&msc->max_lun;
        size = 1;
        break;
    case USB_MSC_REQ_RESET:
    default:
        return VSF_ERR_NOT_SUPPORT;
    }
    ctrl_handler->trans.pchBuffer = buffer;
    ctrl_handler->trans.nSize = size;
    return VSF_ERR_NONE;
}

#endif      // VSF_USE_USB_DEVICE && VSF_USE_USB_DEVICE_MSC
