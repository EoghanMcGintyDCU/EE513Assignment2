#include <iostream>
#include <stdio.h> 
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <unistd.h> 
#include <linux/i2c-dev.h>
#include <map>
#include <string>

#include "RTC.h"

#define BUFFER_SIZE 	19      //0x00 to 0x12
#define REG_0		0x0
#define REG_HOURS	0x2
#define REG_DAY		0x3
#define REG_ALARM_1	0x7
#define REG_ALARM_2	0xB
#define REG_CONTROL	0xE
#define REG_STATUS	0xF
#define REG_TEMP_MSB 	0x11
#define REG_TEMP_LSB 	0x12

#define BIT7_ON_MASK 	0x80
#define BIT7_OFF_MASK 	0x7F
#define ALARM_DAY_MASK 	0x40
#define INTCN_MASK	0x4
#define A2IE_MASK	0x2
#define FREQ_MASK	0xE7
#define AI_FLAGS_MASK	0b00
#define HOURS_MASK	0xBF

std::map<int, std::string> day_map {{1, "Monday"},{2,"Tuesday"},{3, "Wednesday"},
	{4, "Thursday"},{5, "Friday"},{6, "Saturday"}, {7,"Sunday"}};

RTC::RTC(){}

RTC::~RTC(){}

// the time is in the registers in encoded decimal form
int bcdToDec(char b) { return (b/16)*10 + (b%16); }
char decToBcd(int i) { return (i/10)*16 + (i%10); } 

int readI2C(char reg_addr, char* buf, size_t size){
	int file;
	if((file=open("/dev/i2c-2", O_RDWR)) < 0){
	  	return -1;
	}
	if(ioctl(file, I2C_SLAVE, 0x68) < 0){
	  	return -1;
	}
	char addrBuffer[1] = {reg_addr};
	if(write(file, addrBuffer, 1) != 1){
		return -1;
	}
	if(read(file, buf, size)!=size){
		return -1;
	}
	close(file);
	return 0;
}

// First byte in buf must be address writing to
int writeI2C(char* buf, size_t size){
	int file;
	if((file=open("/dev/i2c-2", O_RDWR)) < 0){
		return -1;
	}
	if(ioctl(file, I2C_SLAVE, 0x68) < 0){
	  	return -1;
	}
	if(write(file, buf, size)!=size){
		return -1;
	}
	close(file);
	return 0;
}

int writeReg(char reg_addr, char mask, char write_val){
		
	char read_buf[1];
	if(readI2C(reg_addr, read_buf, 1)<0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	read_buf[0] &= mask;
	read_buf[0] |= write_val;

	char write_buf[2];
	write_buf[1] = read_buf[0];
		
	write_buf[0] = reg_addr;

	if(writeI2C(write_buf, 2) < 0){
		std::cerr << "write failed" << std::endl;
		return -1;
	}
	
	return 0;
}

int RTC::displayTime(){
	char buf[BUFFER_SIZE];
	if(readI2C(REG_0, buf, BUFFER_SIZE)<0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	printf("The RTC time is %02d:%02d:%02d\n", bcdToDec(buf[2]),
	     bcdToDec(buf[1]), bcdToDec(buf[0]));
	return 0;
}

int RTC::displayDate(){
	char buf[BUFFER_SIZE];
	if(readI2C(REG_0, buf, BUFFER_SIZE)<0){
		std::cerr<< "read failed" << std::endl;
		return -1;
	}

	printf("The RTC date is %02d/%02d/%02d\n", bcdToDec(buf[4]),
		bcdToDec(buf[5]), bcdToDec(buf[6]));
	return 0;
}

int RTC::displayTemperature(){
	char buf[BUFFER_SIZE];
	if(readI2C(REG_0, buf, BUFFER_SIZE)<0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	printf("The RTC temperature is %d.%02d Deg C\n", buf[REG_TEMP_MSB], buf[REG_TEMP_LSB]);
	return 0;
}

int RTC::setDate(int secs, int mins, int hours, int day, int date, int month, int year){
	char buf[8];
	buf[1] = decToBcd(secs);
	buf[2] = decToBcd(mins);
	buf[3] = decToBcd(hours);
	buf[4] = decToBcd(day);
	buf[5] = decToBcd(date);
	buf[6] = decToBcd(month);
	buf[7] = decToBcd(year);
	
	buf[0] = REG_0;

	if(writeI2C(buf, 8) < 0){
		std::cerr << "write failed" << std::endl;
		return -1;
	}
	
	std::cout << "Date Set" << std::endl;
	return 0;
}

int RTC::setAlarm1(int secs, int mins, int hours){
	char buf[5];
	buf[1] = decToBcd(secs) & BIT7_OFF_MASK;
	buf[2] = decToBcd(mins) & BIT7_OFF_MASK;
	buf[3] = decToBcd(hours) & BIT7_OFF_MASK;
	buf[4] = BIT7_ON_MASK;
	
	buf[0] = REG_ALARM_1;

	if(writeI2C(buf, 5) < 0){
		std::cerr << "write failed" << std::endl;
		return -1;
	}

	std::cout << "Alarm 1 set" << std::endl;

	return 0;
}

int RTC::setAlarm2(int mins, int hours, int day){
	char buf[4];
	buf[1] = decToBcd(mins) & BIT7_OFF_MASK;
	buf[2] = decToBcd(hours) & BIT7_OFF_MASK;
	buf[3] = decToBcd(day) & BIT7_OFF_MASK | ALARM_DAY_MASK;
	
	buf[0] = REG_ALARM_2;

	if(writeI2C(buf, 4) < 0){
		std::cerr << "write failed" << std::endl;
		return -1;
	}

	std::cout << "Alarm 2 set" << std::endl;

	return 0;
}

int RTC::readAlarm1(){
	char buf[3];
	if(readI2C(REG_ALARM_1, buf, 3)<0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	printf("The RTC Alarm 1 is set to %02d:%02d:%02d\n", bcdToDec(buf[2]),
		bcdToDec(buf[1]), bcdToDec(buf[0]));

	return 0;
}

int RTC::readAlarm2(){
	
	char buf[3];
	if(readI2C(REG_ALARM_2, buf, 3)<0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	printf("The RTC Alarm 2 is set to %02d:%02d on %d day of week\n", bcdToDec(buf[1]),
		bcdToDec(buf[0]), bcdToDec(buf[2] & ~ALARM_DAY_MASK));

	return 0;
}	

int RTC::alarmTest(){

	std::cout << "Setting alarm flags to 0" << std::endl;
	if(writeReg(REG_STATUS, ~(AI_FLAGS_MASK), AI_FLAGS_MASK) < 0){
		return -1;
	}

	std::cout << "Setting INTCN bit and Alarm 2 Interrupt enable on control register" << std::endl;

	if(writeReg(REG_CONTROL, ~(INTCN_MASK | A2IE_MASK), INTCN_MASK | A2IE_MASK) < 0){
		return -1;
	}

	std::cout << "Setting Alarm 2 to interrupt every minute" << std::endl;

	char alarm2_buf[4];
	alarm2_buf[1] = BIT7_ON_MASK | 1;
	alarm2_buf[2] = BIT7_ON_MASK;
	alarm2_buf[3] = BIT7_ON_MASK;
	
	alarm2_buf[0] = REG_ALARM_2;

	if(writeI2C(alarm2_buf, 4) < 0){
		std::cerr << "write failed" << std::endl;
		return -1;
	}

	return 0;
}

int RTC::setSquareWave(Freq freq){
	std::cout << "Setting RS1 & RS2 on control register" << std::endl;
	
	if(writeReg(REG_CONTROL, FREQ_MASK & ~INTCN_MASK, (freq << 3)) < 0){
		return -1;
	}
	
	return 0;	
}

int RTC::novel(){
	std::cout << "Changing clock to 12 hour" << std::endl;

	char buf[1];
	if(readI2C(REG_HOURS, buf, 1)<0){
		std::cerr << "read failed" <<std::endl;
		return -1;	
	}

	int hours = bcdToDec(buf[0]);
	char am_pm = 0x0;
	if(hours > 12){
		hours %= 12;
		am_pm = 0b1 << 5;
	}

	if(hours == 12){
		am_pm = 0b1 << 5;
	}
	
	if(writeReg(REG_HOURS, HOURS_MASK, (0b1 << 6) | decToBcd(hours) | am_pm) < 0){
		return -1;
	}

	if(readI2C(REG_DAY, buf, 1) < 0){
		std::cerr << "read failed" << std::endl;
		return -1;
	}

	std::cout << "The day is " << day_map[buf[0]] << std::endl;

	return 0;
}

