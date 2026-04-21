/*
 * menu.c
 *
 *  Created on: Nov 24, 2025
 *      Author: USER
 */


#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "dma_printf.h"
#include "dma_scanf.h"
#include "log.h"
#include "w25q_mem.h"
#include "project_info.h"
#include "stdlib.h"

#include "lfs.h"
#include "littlefs_qspi.h"

/* USER CODE END Includes */

extern struct lfs_config qspi_lfs_cfg;
extern RTC_HandleTypeDef hrtc;

void Set_RTC_Time(void);
void Set_RTC_Date(void);

void menu1_option_1(void);
void menu1_option_2(void);
void menu1_option_3(void);
void menu1_option_4(void);
void menu1_option_5(void);
void menu1_option_6(void);
void menu1_option_7(void);
void menu1_option_8(void);
void menu2_option_9(void);
void menu2_option_10(void);
void menu2_option_11(void);
void menu2_option_12(void);
void menu2_option_13(void);
void menu2_option_14(void);

void menu1_exit(void);

void printHex(unsigned char *buffer, int buffer_size);

// Menu structure
typedef struct {
    const char *name;
    void (*handler)(void);
} MenuItem;

// Menu items table
MenuItem menu1[] = {
    {"Read W25Q ID",                    menu1_option_1},
    {"Read W25Q FullID",                menu1_option_2},
	{"Read W25Q JEDECID",               menu1_option_3},
	{"Read W25Q UID",                   menu1_option_4},
	{"Read W25Q Page",                  menu1_option_5},
	{"Erase W25Q Sector",               menu1_option_6},
	{"Read/Write Page",                 menu1_option_7},
	{"Erase W25Q Chip",                 menu1_option_8},
    {"Exit",                            menu1_exit}
};

// Menu items table
MenuItem menu2[] = {
	{"littlefs_qspi_Read (4KB Page)",   menu2_option_9},
	{"littlefs_qspi_Write (256B Page)", menu2_option_10},
	{"littlefs_qspi_Erase (4KB Block)", menu2_option_11},
	{"littleFS Format",                 menu2_option_12},
	{"littleFS List root directory",    menu2_option_13},
	{"Setting Date/Time",               menu2_option_14},
    {"Exit",                            menu1_exit}
};


#define MENU1_COUNT (sizeof(menu1) / sizeof(MenuItem))
#define MENU2_COUNT (sizeof(menu2) / sizeof(MenuItem))

u8_t id = 0;

// Declare LFS as global variable so it can used in different functions.
extern lfs_t lfs;

// Menu option implementations
void menu1_option_1(void) {
	W25Q_STATE state;
	while (W25Q_IsBusy() == W25Q_BUSY)
		HAL_Delay(1);
    state = W25Q_ReadID(&id);

	if (state != W25Q_OK)
	  printf("W25Q Read_ID() failed\n\r");
	else
	  printf("W25Q ID: %X\n\r\n\r", id);


}

void menu1_option_2(void) {
	u8_t buffer[8];
	W25Q_STATE state;

	while (W25Q_IsBusy() == W25Q_BUSY)
		HAL_Delay(1);
	state = W25Q_ReadFullID(buffer);

	if (state != W25Q_OK)
	  printf("W25Q Read_ID() failed\n\r");
	else
	  printf("W25Q Full ID: %X %X %X\n\r\n\r", buffer[0], buffer[1], buffer[2]);


}


void menu1_option_3(void) {
	u8_t buffer[8];
	W25Q_STATE state;

	while (W25Q_IsBusy() == W25Q_BUSY)
		HAL_Delay(1);
	state = W25Q_ReadJEDECID(buffer);

	if (state != W25Q_OK)
	  printf("W25Q W25Q_ReadJEDECID() failed\n\r");
	else
	  printf("W25Q JEDEC ID: %X %X %X\n\r\n\r", buffer[0], buffer[1], buffer[2]);



}

void menu1_option_4(void) {
	u8_t buffer[8];
	W25Q_STATE state;

	while (W25Q_IsBusy() == W25Q_BUSY)
		HAL_Delay(1);
	state = W25Q_ReadUID(buffer);

	if (state != W25Q_OK)
	  printf("W25Q W25Q_UID() failed\n\r");
	else
	  printf("W25Q Unique ID: %X %X %X %X %X %X %X %X\n\r\n\r",
			  buffer[0], buffer[1], buffer[2], buffer[3],
			  buffer[4], buffer[5], buffer[6], buffer[7]);

}

void menu1_option_5(void) {
	uint32_t  page_num;
    uint8_t buf[256];
    W25Q_STATE state;

	printf("Enter page # (0-65535) ");
	scanf("%ld", &page_num);
	printf("\n\r");
    printf("Read 256 bytes from Page %ld\n\r", page_num);
	state = W25Q_ReadData(buf, 256, 0, page_num);
	if (state != W25Q_OK)
	  printf("W25Q W25Q_ReadData(buf, 256, 0, %ld) failed\n\r", page_num);
	else
	  printHex(buf, 256);



}

void menu1_option_6(void) {
	// Erase 4K Sector
	uint32_t  sec_num;
	W25Q_STATE state;

	printf("Enter 4K Sector (0-4095)# ");
	scanf("%ld", &sec_num);
	printf("\n\r");
	state = W25Q_EraseSector(sec_num);
	if (state != W25Q_OK)
	  printf("W25Q_EraseSector(%ld) failed\n\r", sec_num);
}


void menu1_option_7(void) {

  uint8_t buf[256];
  uint32_t  page_num;
  W25Q_STATE state;

	printf("Enter Page # (0-65535)# ");
	scanf("%ld", &page_num);
	printf("\n\r");

	  for (int i=0; i<256;i++) {
		  buf[i] = i;
	  }

	  state = W25Q_ProgramData(buf, 256, 0, page_num) ;
	  if (state != W25Q_OK)
	  	  printf("W25Q W25Q_ProgramRaw() failed\n\r");

	  printf("Read 256 bytes from flash address: %ld\n\r", page_num);
	  state = W25Q_ReadData(buf, 256, 0, page_num);
	  if (state != W25Q_OK)
	  	  printf("W25Q W25Q_ReadData(buf, 256, 0, %ld) failed\n\r", page_num);
	  else
		  printHex(buf, 256);
}

void menu1_option_8(void) {
	// Erase Chip
	W25Q_STATE state;

	state = W25Q_EraseChip();

	if (state != W25Q_OK)
	  printf("W25Q_EraseChip failed\n\r");
}

void menu2_option_9(void) {
	uint8_t buf[512];
//	qspi_read(const struct lfs_config *c, lfs_block_t block,
//	              lfs_off_t off, void *buffer, lfs_size_t size)

	uint32_t  block_num;

	printf("Enter Block # (0-4095)# ");
	scanf("%ld", &block_num);

	qspi_read(&qspi_lfs_cfg, block_num, 0, buf, 512);

	printf("Read from block %ld offset %d\n\r", block_num, 0);
	printHex(buf, 256);
	printf("\n\r");
	printHex(buf+256, 256);

}
void menu2_option_10(void) {
	uint8_t buffer[256];
	uint32_t  block_num;

	printf("Enter Block # (0-4095)# ");
	scanf("%ld", &block_num);
	printf("\n\r");
	for(int i=0; i<256; i++)
		buffer[i] = i;
	qspi_prog(&qspi_lfs_cfg, block_num, 0, buffer, 256);
	printf("Write 256 bytes to block %ld offset %d\n\r", block_num, 0);
}


void menu2_option_11(void) {
	uint32_t  block_num;

	printf("Enter Block# (0-4095) : ");
	scanf("%ld", &block_num);
	printf("\n\r");

	qspi_erase(&qspi_lfs_cfg, block_num) ;

	printf("Erase block# %ld\n\r", block_num);
}


void menu2_option_12(void) {
//  lfs_t lfs;
  int rc;
  rc = lfs_format(&lfs, &qspi_lfs_cfg);
  printf("LittleFS Formatted, RC=%d\n\r", rc);
}

void menu2_option_13(void) {
//	lfs_t lfs;
  lfs_dir_t dir;
  struct lfs_info info;
  int rc;

  rc = lfs_mount(&lfs, &qspi_lfs_cfg);
  if (rc != 0) {
	  printf("lfs_mount() failed, RC=%d\n\r", rc);
      rc = lfs_format(&lfs, &qspi_lfs_cfg);
	  printf("lfs_format() failed, RC=%d\n\r", rc);
      rc = lfs_mount(&lfs, &qspi_lfs_cfg);
	  printf("lfs_mount() failed, RC=%d\n\r", rc);
  } else {
	  printf("LFS_mount() success\n\r");
  }

  int err = lfs_dir_open(&lfs, &dir, "/");
  if (err) {
	printf("lfs_dir_open failed, rc = %d\n\r", err);
	return;
  }

  while (true) {
	int res = lfs_dir_read(&lfs, &dir, &info);
	if (res < 0) {
	   	printf("lfs_dir_read failed, rc = %d\n\r", res);// Error occurred
	    break;
	}
	if (res == 0) {
	    // No more entries
	    break;
	}

	// Print entry name and type
	printf("%s (%s)\n\r", info.name,
	    (info.type == LFS_TYPE_DIR) ? "dir" : "file");

  }
}


void menu2_option_14(void) {
	char  setting[2];
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	printf("RTC Time: %02d:%02d:%02d\n\r", sTime.Hours, sTime.Minutes, sTime.Seconds);
	printf("RTC Date: %02d/%02d/%02d\n\r", sDate.Date, sDate.Month, sDate.Year);

	printf("Setting Date/Time (y/N)  : ");
	scanf("%s", setting);
	printf("\n\r");


	if (setting[0] != 'Y' && setting[0] != 'y')
		return;
    Set_RTC_Time();
    Set_RTC_Date();

}

void menu1_exit(void) {
    printf("Exiting menu...\r\n");

}

// Main menu loop
void run_menu1(void) {
  char input[8];
  int choice;

  while (1) {
    printf("\r\n=== W25Q128 Menu ===\r\n");
    for (size_t i = 0; i < MENU1_COUNT; i++) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%d) %s\r\n", (int)(i + 1), menu1[i].name);
      printf("%s", buf);
    }
    printf("Select option (timeout 10 Secs) : ");

    int incoming = get_char_with_timeout(10000) ;

    if (incoming == -2) {
      choice = MENU1_COUNT;
      printf("\n\rInput timeout, quit menu\n\r");
    }
    else if (incoming != -1) {
      input[0] = incoming;
      input[1] = '\0';

      printf("%s\n\r", input);
      choice = atoi(input);
    }

   	if (choice >= 1 && choice <= MENU1_COUNT) {
      menu1[choice - 1].handler();
      if (choice == MENU1_COUNT) {
      	break;
      } else {
    	  if (choice < 1 && choice > MENU1_COUNT) {
            printf("Invalid choice. Try again.\r\n");
    	  }
      }
    }
  }
}



// Main menu loop
void run_menu2(void) {
  char input[8];
  int choice;
  while (1) {
    printf("\r\n=== LittleFS Menu ===\r\n");
    for (size_t i = 0; i < MENU2_COUNT; i++) {
      char buf[64];
      snprintf(buf, sizeof(buf), "%d) %s\r\n", (int)(i + 1), menu2[i].name);
      printf("%s", buf);
    }
    printf("Select option: ");

    int incoming = get_char_with_timeout(10000) ;

    if (incoming == -2) {
      choice = MENU2_COUNT;
      printf("\n\rInput timeout, quit menu\n\r");
    }
    else if (incoming != -1) {
      input[0] = incoming;
      input[1] = '\0';

      printf("%s\n\r", input);
      choice = atoi(input);
    }

   	if (choice >= 1 && choice <= MENU2_COUNT) {
      menu2[choice - 1].handler();
      if (choice == MENU2_COUNT) {
    	break;
      } else {
        if (choice < 1 && choice > MENU2_COUNT) {
          printf("Invalid choice. Try again.\r\n");
        }
      }
    }
#if 0
        scanf("%s", input);
        printf("%s\n\r", input);
        int choice = atoi(input);

        if (choice >= 1 && choice <= MENU2_COUNT) {
            menu2[choice - 1].handler();
            if (choice == MENU2_COUNT)
            	break;
        } else {
            printf("Invalid choice. Try again.\r\n");
        }
#endif
    }
}

