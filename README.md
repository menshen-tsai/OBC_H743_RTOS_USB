echo "# OBC_H743_RTOS_USB" >> README.md
git init
git add README.md
git commit -m "first commit"
git branch -M main
git remote add origin https://github.com/menshen-tsai/OBC_H743_RTOS_USB.git
git push -u origin main

To update:

git push

//////////////////////////////////////////////////////////////
2026/04/14

Implementing USB CDC (Communication Device Class) on an STM32H7

1. The Setup (CubeMX Configuration)To get USB CDC running at Full Speed (FS) or 
   High Speed (HS), you need to configure three specific areas:
   A. Middleware
      Go to Connectivity $\rightarrow$ USB_OTG_FS (or HS).
      Set Mode to Device_Only.
      Go to Middleware $\rightarrow$ USB_DEVICE.
      Class for FS/HS IP: Select Communication Device Class (Virtual Port Com).
   B. Clock Configuration
   The USB peripheral must have a precise 48MHz clock.
      In the Clock Tree, ensure the USB Circuit is receiving 48MHz. 
      On an H7, you typically use the HSI48 (High-Speed Internal 48MHz) 
      oscillator with CRS (Clock Recovery System) enabled to avoid 
      needing an external crystal for USB.
   C. Specific H7 Settings
      VCORE Voltage: Ensure the Power Regulator voltage scale is sufficient 
      for the clock speed you've chosen.
      Caches: If using High Speed with DMA, you may need to manage the 
      M7 D-Cache (Data Cache) to avoid memory coherency issues. For a 
      basic example, keep it simple and use standard buffers.
      
2. The Code Example (Transmitting Data)

   Once you generate the code, CubeMX creates a file called usbd_cdc_if.c. 
   To send data from the STM32 to your PC, you use the CDC_Transmit_FS function.
   
   Sending a "Hello World" in main.c:
   
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h" // Include the CDC interface header
#include <string.h>
/* USER CODE END Includes */

// Inside the while(1) loop
while (1)
{
    char *msg = "STM32H7 USB CDC Active\r\n";
    
    // Transmit via USB
    // Parameters: (Buffer pointer, length)
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    
    HAL_Delay(1000); // Wait 1 second
}


3. Handling Incoming Data (Receiving)
To process data sent from the PC to the STM32, you must modify the callback function 
inside usbd_cdc_if.c.

Modify CDC_Receive_FS in usbd_cdc_if.c:

static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */
  
  // Example: Echo the data back to the PC
  CDC_Transmit_FS(Buf, *Len);
  
  // Or: Set a flag to process this buffer in your main loop
  // My_Custom_Receive_Handler(Buf, *Len);

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, Buf);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  return (USBD_OK);
  /* USER CODE END 6 */
}


4. Critical Tips for STM32H7

   1. Heap Size: USB CDC requires a decent amount of heap memory to manage 
      buffers. In your Project Settings (Linker script), ensure your Minimum 
      Heap Size is at least 0x600 or 0x1000.

   2. The USB Connector: If you are using a custom board, ensure you have a 
      1.5k$\Omega$ pull-up resistor on the D+ line (or ensure the STM32's 
      internal pull-up is enabled in CubeMX).
      
   3. VBUS Sensing: If your hardware doesn't connect the 5V VBUS pin to the STM32, 
      you must disable VBUS Sensing in the USB_OTG_FS settings in CubeMX, 
      or the device will never "enumerate" (appear on your computer).   
      
      
5. Adjust Clock such that USB clock is EXACTLY 48MHz
   Manually adjust clock settings,
   
   DIVM1  /1
   DIVN1  x120
   DIVP1  /2           480MHz    To SYSCLK and all other Peripherals
   DIVQ1  /2
   DIVR1  /2
   
   DIVM2  /1
   DIVN2  x30
   DIVP2  /2           120MHz
   DIVQ2  /2
   DIVR2  /2
   
   DIVM3   /4
   DIVN3  x144
   DIVP3  /2           144MHz
   DIVQ3  /6           48MHz
   DIVR3  /2           144MHz
   
   UART2,3,4,5,7,8     PLL2Q      120MHz
   USB                 PLL3Q      48MHz 
   FDCAN               PLL2Q      120MHz      