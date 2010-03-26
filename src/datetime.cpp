// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/numbers.h>
#include <ucommon/datetime.h>
#include <ucommon/thread.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace UCOMMON_NAMESPACE;

#ifdef __BORLANDC__
using std::time_t;
using std::tm;
using std::localtime;
#endif

#ifdef	HAVE_LOCALTIME_R

struct tm *DateTime::getLocaltime(time_t *now)
{	
	struct tm *result, *dt = new struct tm;
	time_t tmp;

	if(!now) {
		now = &tmp;
		time(&tmp);
	}

	result = localtime_r(now, dt);
	if(result)
		return result;
	delete dt;
	return NULL;
}

struct tm *DateTime::getGMT(time_t *now)
{	
	struct tm *result, *dt = new struct tm;
	time_t tmp;

	if(!now) {
		now = &tmp;
		time(&tmp);
	}

	result = gmtime_r(now, dt);
	if(result)
		return result;
	delete dt;
	return NULL;
}

void DateTime::release(struct tm *dt)
{
	if(dt)
		delete dt;
}

#else
static Mutex locking;

struct tm *DateTime::getLocaltime(time_t *now)
{	
	struct tm *dt;
	time_t tmp;

	if(!now) {
		now = &tmp;
		time(&tmp);
	}

	locking.acquire();
	dt = localtime(now);
	if(dt)
		return dt;
	locking.release();
	return NULL;
}

struct tm *DateTime::getGMT(time_t *now)
{	
	struct tm *dt;
	time_t tmp;

	if(!now) {
		now = &tmp;
		time(&tmp);
	}

	locking.acquire();
	dt = gmtime(now);
	if(dt)
		return dt;
	locking.release();
	return NULL;
}

void DateTime::release(struct tm *dt)
{
	if(dt)
		locking.release();
}

#endif

Date::Date()
{
	struct tm *dt = DateTime::getLocaltime();

	toJulian(dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);
	DateTime::release(dt);
}

Date::Date(struct tm *dt)
{
	toJulian(dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);
}

Date::Date(time_t tm)
{
	struct tm *dt = DateTime::getLocaltime(&tm);
	toJulian(dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);
	DateTime::release(dt);
}

Date::Date(const char *str, size_t size)
{
	set(str, size);
}

Date::~Date()
{
}

void Date::set(const char *str, size_t size)
{
	struct tm *dt = DateTime::getLocaltime();
	int year = 0;
	const char *mstr = str;
	const char *dstr = str;

	if(!size)
		size = strlen(str);
//0000
	if(size == 4) {
		year = dt->tm_year + 1900;
		mstr = str;
		dstr = str + 2;
	}
//00/00
	else if(size == 5) {
		year = dt->tm_year + 1900;
		mstr = str;
		dstr = str + 3;
	}
//000000
	else if(size == 6) {
		ZNumber nyear((char*)str, 2);
		year = ((dt->tm_year + 1900) / 100) * 100 + nyear();
		mstr = str + 2;
		dstr = str + 4;
	}
//00000000
	else if(size == 8 && str[2] >= '0' && str[2] <= '9' && str[5] >= '0' && str[5] <= '9') {
		ZNumber nyear((char*)str, 4);
		year = nyear();
		mstr = str + 4;
		dstr = str + 6;
	}
//00/00/00
	else if(size == 8) {
		ZNumber nyear((char*)str, 2);
		year = ((dt->tm_year + 1900) / 100) * 100 + nyear();
		mstr = str + 3;
		dstr = str + 6;
	}

//0000/00/00
	else if(size == 10) {
		ZNumber nyear((char*)str, 4);
		year = nyear();
		mstr = str + 5;
		dstr = str + 8;
	}
	else {
		julian = 0x7fffffffl;
		DateTime::release(dt);
		return;
	}

	DateTime::release(dt);
	ZNumber nmonth((char*)mstr, 2);
	ZNumber nday((char*)dstr, 2);
	toJulian(year, nmonth(), nday());
}

Date::Date(int year, unsigned month, unsigned day)
{
	toJulian(year, month, day);
}

void Date::update(void)
{
}

bool Date::isValid(void) const
{
	if(julian == 0x7fffffffl)
		return false;
	return true;
}

char *Date::get(char *buf) const
{
	fromJulian(buf);
	return buf;
}

time_t Date::getTime(tm* dt) const
{
	char buf[11];
	memset(dt, 0, sizeof(tm));
	fromJulian(buf);
	Number nyear(buf, 4);
	Number nmonth(buf + 5, 2);
	Number nday(buf + 8, 2);

	dt->tm_year = nyear() - 1900;
	dt->tm_mon = nmonth() - 1;
	dt->tm_mday = nday();

	return mktime(dt); // to fill in day of week etc.
}

time_t Date::getTime(void) const
{
	struct tm dt;
	return getTime(&dt);
}

int Date::getYear(void) const
{
	char buf[11];
	fromJulian(buf);
	Number num(buf, 4);
	return num();
}

unsigned Date::getMonth(void) const
{
	char buf[11];
	fromJulian(buf);
	Number num(buf + 5, 2);
	return num();
}

unsigned Date::getDay(void) const
{
	char buf[11];
	fromJulian(buf);
	Number num(buf + 8, 2);
	return num();
}

unsigned Date::getDayOfWeek(void) const
{
	return (unsigned)((julian + 1l) % 7l);
}

String Date::operator()() const
{
	char buf[11];

	fromJulian(buf);
	String date(buf);

	return date;
}

long Date::get(void) const
{
	char buf[11];
	fromJulian(buf);
	return atol(buf) * 10000 + atol(buf + 5) * 100 + atol(buf + 8);
}

Date& Date::operator++()
{
	++julian;
	update();
	return *this;
}

Date& Date::operator--()
{
	--julian;
	update();
	return *this;
}

Date Date::operator+(long val)
{
	Date result = *this;
	result += val;
	return result;
}

Date Date::operator-(long val)
{
	Date result = *this;
	result -= val;
	return result;
}

Date& Date::operator+=(long val)
{
	julian += val;
	update();
	return *this;
}

Date& Date::operator-=(long val)
{
	julian -= val;
	update();
	return *this;
}

int Date::operator==(const Date &d)
{
	return julian == d.julian;
}

int Date::operator!=(const Date &d)
{
	return julian != d.julian;
}

int Date::operator<(const Date &d)
{
	return julian < d.julian;
}

int Date::operator<=(const Date &d)
{
	return julian <= d.julian;
}

int Date::operator>(const Date &d)
{
	return julian > d.julian;
}

int Date::operator>=(const Date &d)
{
	return julian >= d.julian;
}

void Date::toJulian(long year, long month, long day)
{
	julian = 0x7fffffffl;

	if(month < 1 || month > 12 || day < 1 || day > 31 || year == 0)
		return;

	if(year < 0)
		year--;

	julian = day - 32075l +
		1461l * (year + 4800l + ( month - 14l) / 12l) / 4l +
		367l * (month - 2l - (month - 14l) / 12l * 12l) / 12l -
		3l * ((year + 4900l + (month - 14l) / 12l) / 100l) / 4l;
}

void Date::fromJulian(char *buffer) const
{
// The following conversion algorithm is due to
// Henry F. Fliegel and Thomas C. Van Flandern:

	ZNumber nyear(buffer, 4);
	buffer[4] = '-';
	ZNumber nmonth(buffer + 5, 2);
	buffer[7] = '-';
	ZNumber nday(buffer + 8, 2);

	double i, j, k, l, n;

	l = julian + 68569.0;
	n = int( 4 * l / 146097.0);
	l = l - int( (146097.0 * n + 3)/ 4 );
	i = int( 4000.0 * (l+1)/1461001.0);
	l = l - int(1461.0*i/4.0) + 31.0;
	j = int( 80 * l/2447.0);
	k = l - int( 2447.0 * j / 80.0);
	l = int(j/11);
	j = j+2-12*l;
	i = 100*(n - 49) + i + l;
	nyear = int(i);
	nmonth = int(j);
	nday = int(k);

	buffer[10] = '\0';
}

Time::Time()
{
	struct tm *dt = DateTime::getLocaltime();
	toSeconds(dt->tm_hour, dt->tm_min, dt->tm_sec);
	DateTime::release(dt);
}

Time::Time(struct tm *dt)
{
	toSeconds(dt->tm_hour, dt->tm_min, dt->tm_sec);
}

Time::Time(time_t tm)
{
	struct tm *dt = DateTime::getLocaltime(&tm);
	toSeconds(dt->tm_hour, dt->tm_min, dt->tm_sec);
	DateTime::release(dt);
}

Time::Time(char *str, size_t size)
{
	set(str, size);
}

Time::Time(int hour, int minute, int second)
{
	toSeconds(hour, minute, second);
}

Time::~Time()
{
}

bool Time::isValid(void) const
{
	if(seconds == -1)
		return false;
	return true;
}

char *Time::get(char *buf) const
{
	fromSeconds(buf);
	return buf;
}

time_t Time::getTime(void) const
{
	return static_cast<time_t>(seconds);
}

int Time::getHour(void) const
{
	char buf[7];
	fromSeconds(buf);
	ZNumber num(buf, 2);
	return num();
}

int Time::getMinute(void) const
{
	char buf[7];
	fromSeconds(buf);
	ZNumber num(buf + 2, 2);
	return num();
}

int Time::getSecond(void) const
{
	char buf[7];
	fromSeconds(buf);
	ZNumber num(buf + 4, 2);
	return num();
}

void Time::update(void)
{
	seconds = abs(seconds % 86400l); 
}

void Time::set(char *str, size_t size)
{
	int sec = 00;

	if(!size)
		size = strlen(str);

//00:00
	if (size == 5) {
		sec = 00;
	}
//00:00:00
	else if (size == 8) {
		ZNumber nsecond(str + 6, 2);
		sec = nsecond();
	}
	else {
		seconds = -1;
		return;
	}

	ZNumber nhour(str, 2);
	ZNumber nminute(str+3, 2);
	toSeconds(nhour(), nminute(), sec);
}


String Time::operator()() const
{
	char buf[7];

	fromSeconds(buf);
	String strTime(buf);

	return strTime;
}

long Time::get(void) const
{
	return seconds;
}

Time& Time::operator++()
{
	++seconds;
	update();
	return *this;
}

Time& Time::operator--()
{
	--seconds;
	update();
	return *this;
}

Time& Time::operator+=(long val)
{
	seconds += val;
	update();
	return *this;
}

Time& Time::operator-=(long val)
{
	seconds -= val;
	update();
	return *this;
}

Time Time::operator+(long val)
{
	Time result = *this;
	result += val;
	return result;
}

Time Time::operator-(long val)
{
	Time result = *this;
	result -= val;
	return result;
}

int Time::operator==(const Time &t)
{
	return seconds == t.seconds;
}

int Time::operator!=(const Time &t)
{
	return seconds != t.seconds;
}

int Time::operator<(const Time &t)
{
	return seconds < t.seconds;
}

int Time::operator<=(const Time &t)
{
	return seconds <= t.seconds;
}

int Time::operator>(const Time &t)
{
	return seconds > t.seconds;
}

int Time::operator>=(const Time &t)
{
	return seconds >= t.seconds;
}

long Time::operator-(const Time &t)
{
	if(seconds < t.seconds)
		return (seconds + 86400l) - t.seconds;
	else
		return seconds - t.seconds;
}

void Time::toSeconds(int hour, int minute, int second)
{
	seconds = -1;

	if (hour > 23 ||minute > 59 ||second > 59)
		return;

	seconds = 3600 * hour + 60 * minute + second;
}

void Time::fromSeconds(char *buffer) const
{
	ZNumber hour(buffer, 2);
	ZNumber minute(buffer + 2, 2);
	ZNumber second(buffer + 4, 2);

	hour = seconds / 3600;
	minute = (seconds - (3600 * hour())) / 60;
	second = seconds - (3600 * hour()) - (60 * minute());
	buffer[6] = '\0';
}


DateTime::DateTime(time_t tm)
{
	struct tm *dt = DateTime::getLocaltime();
	toJulian(dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);
	toSeconds(dt->tm_hour, dt->tm_min, dt->tm_sec);
	DateTime::release(dt);
}

DateTime::DateTime(tm *dt) :
Date(dt), Time(dt)
{}

DateTime::DateTime(const char *a_str, size_t size)
{
	char *timestr;

	if (!size)
		size = strlen(a_str);

	char *str = new char[size+1];
	strncpy(str, a_str, size);
	str[size]=0;

// 00/00 00:00
	if (size ==  11) {
		timestr = str + 6;
		Date::set(str, 5);
		Time::set(timestr, 5);
	}
// 00/00/00 00:00
	else if (size == 14) {
		timestr = str + 9;
		Date::set(str, 8);
		Time::set(timestr,5);
	}
// 00/00/00 00:00:00
	else if (size == 17) {
		timestr = str + 9;
		Date::set(str, 8);
		Time::set(timestr,8);
	}
// 0000/00/00 00:00:00
	else if (size == 19) {
		timestr = str + 11;
		Date::set(str, 10);
		Time::set(timestr,8);
	}
	delete str;
}


DateTime::DateTime(int year, unsigned month, unsigned day,
		   int hour, int minute, int second) :
  Date(year, month, day), Time(hour, minute, second)
{}

DateTime::DateTime() : Date(), Time()
{
	struct tm *dt = DateTime::getLocaltime();
	toSeconds(dt->tm_hour, dt->tm_min, dt->tm_sec);
	toJulian(dt->tm_year + 1900, dt->tm_mon + 1, dt->tm_mday);
	DateTime::release(dt);
}

DateTime::~DateTime()
{
}

bool DateTime::isValid(void) const
{
	return Date::isValid() && Time::isValid();
}

char *DateTime::get(char *buf) const
{
	fromJulian(buf);
	buf[10] = ' ';
	fromSeconds(buf+11);
	return buf;
}

time_t DateTime::get(void) const
{
	char buf[11];
	struct tm dt;
	memset(&dt, 0, sizeof(dt));

	fromJulian(buf);
	ZNumber nyear(buf, 4);
	ZNumber nmonth(buf + 5, 2);
	ZNumber nday(buf + 8, 2);
	dt.tm_year = nyear() - 1900;
	dt.tm_mon = nmonth() - 1;
	dt.tm_mday = nday();

	fromSeconds(buf);
	ZNumber nhour(buf, 2);
	ZNumber nminute(buf + 2, 2);
	ZNumber nsecond(buf + 4, 2);
	dt.tm_hour = nhour();
	dt.tm_min = nminute();
	dt.tm_sec = nsecond();
	dt.tm_isdst = -1;

	return mktime(&dt);
}

DateTime& DateTime::operator=(const DateTime datetime)
{
	julian = datetime.julian;
	seconds = datetime.seconds;

	return *this;
}

DateTime& DateTime::operator+=(long value)
{
	seconds += value;
	julian += (seconds / 86400l);
	seconds %= 86400l;
	Date::update();
	return *this;
}

DateTime& DateTime::operator-=(long value)
{
	seconds -= value;
	if(seconds < 0) {
		julian += (seconds / 86400l);
		seconds = 86400 + (seconds % 86400l);
	}
	Date::update();		
	return *this;
}

int DateTime::operator==(const DateTime &d)
{
	return (julian == d.julian) && (seconds == d.seconds);
}

int DateTime::operator!=(const DateTime &d)
{
	return (julian != d.julian) || (seconds != d.seconds);
}

int DateTime::operator<(const DateTime &d)
{
	if (julian != d.julian) {
		return (julian < d.julian);
	}
	else {
		return (seconds < d.seconds);
	}
}

int DateTime::operator<=(const DateTime &d)
{
	if (julian != d.julian) {
		return (julian < d.julian);
	}
	else {
		return (seconds <= d.seconds);
	}
}

int DateTime::operator>(const DateTime &d)
{
	if (julian != d.julian) {
		return (julian > d.julian);
	}
	else {
		return (seconds > d.seconds);
	}
}

int DateTime::operator>=(const DateTime &d)
{
	if (julian != d.julian) {
		return (julian > d.julian);
	}
	else {
		return (seconds >= d.seconds);
	}
}

bool DateTime::operator!() const
{
	return !(Date::isValid() && Time::isValid());
}


String DateTime::strftime(const char *format) const
{
	char buffer[64];
	size_t last;
	time_t t;
	struct tm *tbp;
	String retval;

	t = get();
	tbp = getLocaltime(&t);
	last = ::strftime(buffer, 64, format, tbp);
	release(tbp);

	buffer[last] = '\0';
	retval = buffer;
	return retval;
}

long DateTime::operator-(const DateTime &dt)
{
	long secs = (julian - dt.julian) * 86400l;
	secs += (seconds - dt.seconds);
	return secs;
}

DateTime DateTime::operator+(long value)
{
	DateTime result = *this; 
	result += value; 
	return result;
}

DateTime DateTime::operator-(long value)
{
	DateTime result = *this; 
	result -= value; 
	return result;
}

DateNumber::DateNumber(char *str) :
Number(str, 10), Date(str, 10)
{}

DateNumber::~DateNumber()
{}

