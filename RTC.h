#include <fstream>

enum Freq
{
	freq1 = 0b00, //1Hz
	freq2 = 0b01, //1.024kHz
	freq3 = 0b10, //4.096kHZ
	freq4 = 0b11  //8.192kHz
};

class RTC
{
public:
	RTC();
	int displayTime();
	int displayDate();
	int displayTemperature();
	int setDate(int secs, int mins, int hours, int day, int date, int month, int year);
	int setAlarm1(int secs, int mins, int hours);
	int setAlarm2(int mins, int hours, int day);
	int readAlarm1();
	int readAlarm2();
	int alarmTest();
	int setSquareWave(Freq freq);
	int novel();
	virtual ~RTC();
};
