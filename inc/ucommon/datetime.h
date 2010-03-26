// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
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

/**
 * Basic classes for manipulating time and date based data, particularly
 * that may be in strings.
 * @file ucommon/datetime.h
 */

#ifndef _UCOMMON_DATETIME_H_
#define	_UCOMMON_DATETIME_H_

#ifndef _UCOMMON_CONFIG_H_
#include <ucommon/platform.h>
#endif

#ifndef _UCOMMON_NUMBERS_H_
#include <ucommon/numbers.h>
#endif

#ifndef	_UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

NAMESPACE_UCOMMON

#ifdef __BORLANDC__
	using std::tm;
	using std::time_t;
#endif

/**
 * The Date class uses a julian date representation of the current
 * year, month, and day.  This is then manipulated in several forms
 * and may be exported as needed.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short julian number based date class.
 */
class __EXPORT Date
{
protected:
	long julian;

protected:
	void toJulian(long year, long month, long day);
	void fromJulian(char *buf) const;

	/**
	 * A method to use to "post" any changed values when shadowing
	 * a mixed object class.  This is used by DateNumber.
	 */
	virtual void update(void);

public:
	Date(time_t tm);
	Date(tm *dt);
	Date(const char *str, size_t size = 0);
	Date(int year, unsigned month, unsigned day);
	Date();
	virtual ~Date();

	int getYear(void) const;
	unsigned getMonth(void) const;
	unsigned getDay(void) const;
	unsigned getDayOfWeek(void) const;
	char *get(char *buffer) const;
	time_t getTime(void) const;
	time_t getTime(tm *buf) const;
	long get(void) const;
	void set(const char *str, size_t size = 0);
	bool isValid(void) const;

	inline operator long() const
		{return get();};

	String operator()() const;
	Date& operator++();
	Date& operator--();
	Date& operator+=(long val);
	Date& operator-=(long val);
	
	Date operator+(long val);
	Date operator-(long val);

	/**
	 * Operator to compute number of days between two dates.
	 * @param offset for computation.
	 * @return number of days difference.
	 */
	inline long operator-(const Date &date)
		{return (julian - date.julian);};

	int operator==(const Date &date);
	int operator!=(const Date &date);
	int operator<(const Date &date);
	int operator<=(const Date &date);
	int operator>(const Date &date);
	int operator>=(const Date &date);

	inline bool operator!() const
		{return !isValid();};

	inline operator bool() const
		{return isValid();};
};

/**
 * The Time class uses a integer representation of the current
 * time.  This is then manipulated in several forms
 * and may be exported as needed.
 *
 * @author Marcelo Dalmas <mad@brasmap.com.br>
 * @short Integer based time class.
 */

class __EXPORT Time
{
protected:
	long seconds;

protected:
	void toSeconds(int hour, int minute, int second);
	void fromSeconds(char *buf) const;
	virtual void update(void);

public:
	Time(time_t tm);
	Time(tm *dt);
	Time(char *str, size_t size = 0);
	Time(int hour, int minute, int second);
	Time();
	virtual ~Time();

	long get(void) const;
	int getHour(void) const;
	int getMinute(void) const;
	int getSecond(void) const;
	char *get(char *buffer) const;
	time_t getTime(void) const;
	tm *get(tm *buf) const;
	void set(char *str, size_t size = 0);
	bool isValid(void) const;

	operator bool();

	inline long operator-(Time &ref)
		{return seconds - ref.seconds;};

	Time operator+(long val);
	Time operator-(long val);

	inline operator long()
		{return get();};

	String operator()() const;
	Time& operator++();
	Time& operator--();
	Time& operator+=(long val);
	Time& operator-=(long val);
	int operator==(const Time &time);
	int operator!=(const Time &time);
	int operator<(const Time &time);
	int operator<=(const Time &time);
	int operator>(const Time &time);
	int operator>=(const Time &time);

	inline bool operator!() const
		{return !isValid();};

	inline operator bool() const
		{return isValid();};
};

/**
 * The Datetime class uses a julian date representation of the current
 * year, month, and day and a integer representation of the current
 * time.  This is then manipulated in several forms
 * and may be exported as needed.
 *
 * @author Marcelo Dalmas <mad@brasmap.com.br>
 * @short Integer based time class.
 */

class __EXPORT DateTime : public Date, public Time
{
public:
  	DateTime(time_t tm);
	DateTime(tm *dt);
	DateTime(const char *str, size_t size = 0);
	DateTime(int year, unsigned month, unsigned day,
		 int hour, int minute, int second);
	DateTime();
	virtual ~DateTime();

	char *get(char *buffer) const;
	time_t get(void) const;
	bool isValid(void) const;

	/**
	 * Operator to compute number of days between two dates.
	 * @param offset for computation.
	 * @return number of days difference.
	 */
	long operator-(const DateTime &date);

	DateTime& operator=(const DateTime datetime);

	inline DateTime& operator+=(long value)
		{Date:operator+=(value); return *this;};
	
	inline DateTime& operator-=(long value)
		{Date::operator-=(value); return *this;};

	inline DateTime operator+(long value)
		{DateTime result = *this; result += value; return result;};

	inline DateTime operator-(long value)
		{DateTime result = *this; result -= value; return result;};

	DateTime& operator+=(const Time &time);
	DateTime& operator-=(const Time &time);

	int operator==(const DateTime&);
	int operator!=(const DateTime&);
	int operator<(const DateTime&);
	int operator<=(const DateTime&);
	int operator>(const DateTime&);
	int operator>=(const DateTime&);

	bool operator!() const;
	operator bool() const;

	String strftime(const char *format) const;

	static struct tm *getLocaltime(time_t *now = NULL);
	static struct tm *getGMT(time_t *now = NULL);
	static void release(struct tm *dt);
};

/**
 * A number class that manipulates a string buffer that is also a date.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short a number that is also a date string.
 */
class __EXPORT DateNumber : public Number, public Date
{
protected:
	void update(void)
		{fromJulian(buffer);};

public:
	DateNumber(char *buffer);
	virtual ~DateNumber();
};

typedef	DateTime	datetime_t;
typedef	Date		date_t;

END_NAMESPACE

#endif
