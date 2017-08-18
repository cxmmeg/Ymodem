/**
  ******************************************************************************
  * @file    STM32F0xx_IAP/src/common.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    29-May-2012
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ymodem_port.h"
//#include "ff.h"

/** @addtogroup STM32F0xx_IAP
  * @{
  */
  
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/********************************************************************************
 *                     Porting APIs
 ********************************************************************************/
typedef enum{
    TRANS_FILE  = 0,
    IAP_IMAGE   = 1,
}YMODEM_ACTION_E;

typedef struct{
    COMM_TYPE_E         ymodem_uart;
    YMODEM_ACTION_E     ymodem_action;
    uint32_t            ymodem_receive_supported_maxsize;
    uint32_t            ymodem_transmit_size;
    
    bool_t              ymodem_file_inited;
    uint32_t            ymodem_file_total_size;
    uint32_t            ymodem_file_handled_size;
    FIL                 ymodem_file_handle;
    uint8_t             ymodem_file_tmppath[FF_MAX_LFN];
}YMODEM_DATA_T;

STATIC YMODEM_DATA_T s_ymodem_data;


PUBLIC void ymodem_init(void){
    memset(&s_ymodem_data, 0, sizeof(YMODEM_DATA_T));
    
    s_ymodem_data.ymodem_uart = COM1;
    comm_enable_rxit(s_ymodem_data.ymodem_uart);
    
    s_ymodem_data.ymodem_action = TRANS_FILE;
    
    switch(s_ymodem_data.ymodem_action){
        case TRANS_FILE:
            s_ymodem_data.ymodem_receive_supported_maxsize = 1024 * 1024 *5;
            s_ymodem_data.ymodem_transmit_size = 0;
            break;
        
        case IAP_IMAGE:
            #warning "MUST implement GOT transmit SIZE!"   
            s_ymodem_data.ymodem_receive_supported_maxsize = 1024 * 1024 *5;
            s_ymodem_data.ymodem_transmit_size = 0;
            break;
        
        default:
            //Not supported
            break;
    }
}
PUBLIC uint32_t ymodem_get_receive_maxsize(void){
    return s_ymodem_data.ymodem_receive_supported_maxsize;
}

PUBLIC uint32_t ymodem_get_transmit_size(void){
    return s_ymodem_data.ymodem_transmit_size;
}

//0 - success ; -1 - fail
PUBLIC int ymodem_recv_start_cb(const char * filename, const uint32_t filesize){
    FRESULT fres;
    const char * prefix = (const char*)"1:/";
    char *filepath = (char*)&s_ymodem_data.ymodem_file_tmppath[0];
    uint32_t filepath_size = sizeof(s_ymodem_data.ymodem_file_tmppath);
    
    switch(s_ymodem_data.ymodem_action){
        case TRANS_FILE:
            if(strlen(prefix) + strlen((char*)filename) >= filepath_size){
                return -1;      //path length too long
            }
            memset(&s_ymodem_data.ymodem_file_tmppath[0], 0, filepath_size);
            filepath = strcat((char*)filepath, prefix);
            filepath = strcat((char*)filepath, (char*)filename);
            
            if(FR_OK != (fres = f_open (&s_ymodem_data.ymodem_file_handle, filepath, FA_WRITE | FA_CREATE_ALWAYS))){
                logd("Ymodem Start: create file(%s) fail(%d)...\n", filepath, fres);
                return -1;
            } else {
                logd("Ymodem Start: create file(%s) ok(%d)...\n", filepath, fres);
                
                s_ymodem_data.ymodem_file_inited = TRUE;
                s_ymodem_data.ymodem_file_total_size = filesize;
                s_ymodem_data.ymodem_file_handled_size = 0;
                return 0;
            }
            //break;
        
        case IAP_IMAGE:
            /* erase user application area */
            //FLASH_If_Erase(APPLICATION_ADDRESS);
            
            break;
        
        default:
            //Not supported
            break;
    }
    return -1;
}

PUBLIC int ymodem_recv_processing_cb(const uint8_t * buffer, const uint32_t buff_size){
    FRESULT fres;
    uint32_t to_write_size = 0;
    uint32_t writted_size = 0;
    
    switch(s_ymodem_data.ymodem_action){
        case TRANS_FILE:
            to_write_size = s_ymodem_data.ymodem_file_total_size - s_ymodem_data.ymodem_file_handled_size;
            to_write_size = (buff_size > to_write_size) ? to_write_size : buff_size;
            
            if(FR_OK != (fres = f_write(&s_ymodem_data.ymodem_file_handle, buffer, to_write_size, &writted_size))){
                logw("Ymodem process: write file(%d - %d) fail(%d)...\n", to_write_size, writted_size, fres);
                return -1;
            } else {
                s_ymodem_data.ymodem_file_handled_size += to_write_size;
                logi("Ymodem process: write file(%d/%d) ok(%d)...\n", s_ymodem_data.ymodem_file_handled_size, 
                            s_ymodem_data.ymodem_file_total_size, fres);
                return 0;
            }
            //break;
        
        case IAP_IMAGE:
            //if (FLASH_If_Write(&flashdestination, (uint32_t*) ramsource, (uint16_t) packet_length/4)  == 0)
            break;
        
        default:
            //Not supported
            break;
    }
    return -1;
}

PUBLIC int ymodem_recv_end_cb(void){
    FRESULT fres;
    FILINFO fno;

    switch(s_ymodem_data.ymodem_action){
        case TRANS_FILE:
            if(TRUE != s_ymodem_data.ymodem_file_inited)
                return -1;
            
            fres = f_close(&s_ymodem_data.ymodem_file_handle);
            logd("Ymodem End: close file res(%d)...\n", fres);
            
            fres = f_stat((const TCHAR *)s_ymodem_data.ymodem_file_tmppath, &fno);
            if(fres != RES_OK){
                logw("Get File Status Fail(%d)...\n", fres);
            } else {
                logi("Get File Status ok(%d), file size(%d) Bytes...\n", fres, fno.fsize);
            }
            
            memset(&s_ymodem_data.ymodem_file_handle, 0, sizeof(FIL));
            memset(&s_ymodem_data.ymodem_file_tmppath[0], 0, sizeof(s_ymodem_data.ymodem_file_tmppath));               
            s_ymodem_data.ymodem_file_total_size = 0;
            s_ymodem_data.ymodem_file_handled_size = 0;
            s_ymodem_data.ymodem_file_inited = FALSE;
            return 0;
            //break;
        
        case IAP_IMAGE:
            /* erase user application area */
            //FLASH_If_Erase(APPLICATION_ADDRESS);
            
            break;
        
        default:
            //Not supported
            break;
    }
    return -1;
}
/**
  * @brief  Test to see if a key has been pressed on the HyperTerminal
  * @param  key: The key pressed
  * @retval 1: Correct
  *         0: Error
  */
uint32_t SerialKeyPressed(uint8_t *key)
{
    uint8_t err;
    uint8_t ret;
    
    //wait 10ms at most
    ret = comm_read_char(s_ymodem_data.ymodem_uart, 2, &err);

    if(COMM_NO_ERR == err){
        *key = ret;
        return 1;
    } else {
        return 0;
    }
}
//fail - 0xff; success -other value
uint8_t SerialReadByte(void){
    uint8_t err;
    uint8_t ret;
    
    //wait 10ms at most
    ret = comm_read_char(s_ymodem_data.ymodem_uart, 10, &err);

    if(COMM_NO_ERR == err){
        return ret;
    } else {
        return 0xff;
    }
}

/**
  * @brief  Print a character on the HyperTerminal
  * @param  c: The character to be printed
  * @retval None
  */
void SerialPutChar(uint8_t c)
{
    comm_send_char(s_ymodem_data.ymodem_uart, c, 10);  
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
