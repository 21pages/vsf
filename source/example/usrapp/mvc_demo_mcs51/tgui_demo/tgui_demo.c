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
#include "vsf.h"

#if VSF_USE_TINY_GUI == ENABLED
#include <stdio.h>
#include "./images/demo_images.h"
#include "./stopwatch/stopwatch.h"
/*============================ MACROS ========================================*/
#define DEMO_OFFSET            0

#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_TOP | VSF_TGUI_ALIGN_LEFT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_TOP | VSF_TGUI_ALIGN_RIGHT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_BOTTOM | VSF_TGUI_ALIGN_LEFT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_BOTTOM | VSF_TGUI_ALIGN_RIGHT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_TOP)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_BOTTOM)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_LEFT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_RIGHT)
//#define DEMO_BACKGROUND_ALIGN   (VSF_TGUI_ALIGN_CENTER)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ LOCAL VARIABLES ===============================*/
static NO_INIT vsf_tgui_t s_tTGUIDemo;
static NO_INIT stopwatch_t s_tMyStopwatch;

/*============================ PROTOTYPES ====================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ IMPLEMENTATION ================================*/

static void vsf_tgui_region_init_with_size(vsf_tgui_region_t* ptRegion, vsf_tgui_size_t* ptSize)
{
    ptRegion->tLocation.iX = 0;
    ptRegion->tLocation.iY = 0;
    ptRegion->tSize = *ptSize;
}


void refresh_my_stopwatch(void)
{
    //vk_tgui_refresh(&s_tTGUIDemo);
}


static fsm_rt_t my_stopwatch_start_stop_on_click(vsf_tgui_button_t* ptNode, vsf_msgt_msg_t* ptMSG)
{
    return fsm_rt_cpl;
}



vsf_err_t tgui_demo_init(void)
{
    NO_INIT static vsf_tgui_evt_t __evt_queue_buffer[16];
    NO_INIT static uint16_t __bfs_buffer[32];
    
    const vsf_tgui_cfg_t cfg = {
        .tEVTQueue = {
            .pObj = __evt_queue_buffer, 
            .nSize = sizeof(__evt_queue_buffer)
        },
        .tBFSQueue = {
            .pObj = __bfs_buffer,
            .nSize = sizeof(__bfs_buffer),
        },
        .ptRootNode = (vsf_tgui_control_t *)&s_tMyStopwatch,
    };
    vsf_err_t err = vk_tgui_init(&s_tTGUIDemo, &cfg);
    
    my_stopwatch_init(&s_tMyStopwatch, &s_tTGUIDemo);

    return err;
}
//
//void vsf_tgui_on_keyboard_evt(vk_keyboard_evt_t* evt)
//{
///*
//    VSF_INPUT_KEYBOARD_GET_KEYCODE(evt)
//    VSF_INPUT_KEYBOARD_IS_DOWN(evt)
//*/
///*
//    vsf_trace(VSF_TRACE_INFO, "\r\n Key %08x ", VSF_INPUT_KEYBOARD_GET_KEYCODE(evt));
//    if (VSF_INPUT_KEYBOARD_IS_DOWN(evt)) {
//        vsf_trace(VSF_TRACE_INFO, "is pressed");
//    } else {
//        vsf_trace(VSF_TRACE_INFO, "is released");
//    }
//*/
////! this block of code is used for test purpose only
//    vsf_tgui_evt_t tEvent = {
//        .tKeyEvt = {
//            .tMSG = VSF_INPUT_KEYBOARD_IS_DOWN(evt)
//                                ? VSF_TGUI_EVT_KEY_DOWN
//                                : VSF_TGUI_EVT_KEY_UP,
//            .hwKeyValue = VSF_INPUT_KEYBOARD_GET_KEYCODE(evt),
//        },
//    };
//
//    vsf_tgui_send_message(&s_tTGUIDemo, tEvent);
//
//    if (!VSF_INPUT_KEYBOARD_IS_DOWN(evt)) {
//        tEvent.tKeyEvt.tMSG = VSF_TGUI_EVT_KEY_PRESSED;
//        vsf_tgui_send_message(&s_tTGUIDemo, tEvent);
//    }
//}


//void vsf_tgui_on_touchscreen_evt(vk_touchscreen_evt_t* ts_evt)
//{
///*
//    vsf_trace(VSF_TRACE_DEBUG, "touchscreen(%d): %s x=%d, y=%d" VSF_TRACE_CFG_LINEEND,
//        VSF_INPUT_TOUCHSCREEN_GET_ID(ts_evt),
//        VSF_INPUT_TOUCHSCREEN_IS_DOWN(ts_evt) ? "down" : "up",
//        VSF_INPUT_TOUCHSCREEN_GET_X(ts_evt),
//        VSF_INPUT_TOUCHSCREEN_GET_Y(ts_evt));
// */
//    //! this block of code is used for test purpose only
//
//    vsf_tgui_evt_t tEvent = {
//        .tPointerEvt = {
//            .tMSG = VSF_INPUT_TOUCHSCREEN_IS_DOWN(ts_evt) 
//                                ?   VSF_TGUI_EVT_POINTER_DOWN 
//                                :   VSF_TGUI_EVT_POINTER_UP,
//            
//            .iX = VSF_INPUT_TOUCHSCREEN_GET_X(ts_evt),
//            .iY = VSF_INPUT_TOUCHSCREEN_GET_Y(ts_evt),
//        },
//    };
//
//    vsf_tgui_send_message(&s_tTGUIDemo, tEvent);
//
//    
//    if (!VSF_INPUT_TOUCHSCREEN_IS_DOWN(ts_evt)) {
//        tEvent.tPointerEvt.tMSG = VSF_TGUI_EVT_POINTER_CLICK;
//        vsf_tgui_send_message(&s_tTGUIDemo, tEvent);
//    }
//}

void vsf_tgui_demo_on_ready(void)
{
    bool bRequirePostEvent = false;
    vsf_sched_safe() {
        if (s_tMyStopwatch.tTask.param.bWaitforRefresh) {
            s_tMyStopwatch.tTask.param.bWaitforRefresh = false;
            bRequirePostEvent = true;
        }
    }

    if (bRequirePostEvent) {
        vsf_eda_post_evt(&(s_tMyStopwatch.tTask.use_as__vsf_eda_t), VSF_EVT_USER);
    }
}

#endif


/* EOF */
