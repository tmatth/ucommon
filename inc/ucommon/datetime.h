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
	/**
	 * Create a julian date from a time_t type.
	 * @param value from time()
	 */
	Date(time_t value);

	/**
	 * Create a julian date from a local or gmt date and time.
	 * @param object from DateTime::glt() or gmt().
	 */
	Date(struct tm *object);

	/**
	 * Create a julian date from a ISO date string of a specified size.
	 * @param pointer to ISO date string.
	 * @param size of date field if not null terminated.
	 */
	Date(const char *pointer, size_t size = 0);

	/**
	 * Create a julian date from an arbitrary year, month, and day.
	 * @param year of date.
	 * @param month of date (1-12).
	 * @param day of month (1-31).
	 */
	Date(int year, unsigned month, unsigned day);

	/**
	 * Construct a new julian date with today's date.
	 */
	Date();

	/**
	 * Destroy julian date object.
	 */
	virtual ~Date();

	/**
	 * Get the year of the date.
	 */
	int getYear(void) const;

	/**
	 * Get the month of the date (1-12).
	 */
	unsigned getMonth(void) const;

	/**
	 * Get the day of the month of the date.
	 */
	unsigned getDay(void) const;

	/**
	 * Get the day of the week (0-7).
	 */
	unsigned getDayOfWeek(void) const;

	/**
	 * Get a ISO string representation of the date (yyyy-mm-dd).
	 * @param buffer to store string.
	 * @return string representation.
	 */
	char *get(char *buffer) const;

	/**
	 * Get a time_t for the julian date if in time_t epoch.
	 * @return time_t or -1 if out of range.
	 */
	time_t getTime(void) const;
	
	/**
	 * Get the julian number for the object.
	 * @return julian number.
	 */
	long get(void) const;

	/**
	 * Set the julian date based on an ISO date string of specified size.
	 * @param pointer to date string field.
	 * @param size of field if not null terminated.
	 */
	void set(const char *pointer, size_t size = 0);

	/**
	 * Check if date is valid.
	 * @return true if julian date is valid.
	 */
	bool isValid(void) const;

	/**
	 * Casting operator to return julian number.
	 * @return julian number.
	 */
	inline operator long() const
		{return get();};

	/**
	 * Expression operator to return an ISO date string for the current
	 * julian date.
	 * @return ISO date string.
	 */
	String operator()() const;

	/**
	 * Increment date by one day.
	 */
	Date& operator++();

	/**
	 * Decrement date by one day.
	 */
	Date& operator--();

	/**
	 * Increment date by offset.
	 * @param offset to add to julian date.
	 */
	Date& operator+=(long offset);

	/**
	 * Decrement date by offset.
	 * @param offset to subtract from julian date.
	 */
	Date& operator-=(long offset);
	
	/**
	 * Add days to julian date in an expression.
	 * @param days to add.
	 */
	Date operator+(long days);

	/**
	 * Subtract days from a julian date in an expression.
	 * @param days to subtract.
	 */
	Date operator-(long days);

	/**
	 * Operator to compute number of days between two dates.
	 * @param date offset for computation.
	 * @return number of days difference.
	 */
	inline long operator-(const Date &date)
		{return (julian - date.julian);};

	/**
	 * Compare julian dates if same date.
	 * @param date to compare with.
	 */
	int operator==(const Date &date);

	/**
	 * Compare julian dates if not same date.
	 * @param date to compare with.
	 */
	int operator!=(const Date &date);

	/**
	 * Compare julian date if less than another date.
	 * @param date to compare with.
	 */
	int operator<(const Date &date);

	/**
	 * Compare julian date if less than or equal to another date.
	 * @param date to compare with.
	 */
	int operator<=(const Date &date);

	/**
	 * Compare julian date if greater than another date.
	 * @param date to compare with.
	 */
	int operator>(const Date &date);

	/**
	 * Compare julian date if greater than or equal to another date.
	 * @param date to compare with.
	 */
	int operator>=(const Date &date);

	/**
	 * Check if julian date is not valid.
	 * @return true if date is invalid.
	 */
	inline bool operator!() const
		{return !isValid();};

	/**
	 * Check if julian date is valid for is() expression.
	 * @return true if date is valid.
	 */
	inline operator bool() const
		{return isValid();};
};

/**
 * The Time class uses a integer representation of the current
 * time.  This is then manipulated in several forms and may be
 * exported as needed.  The time object can represent an instance in
 * time (hours, minutes, and seconds) in a 24 hour period or can
 * represent a duration.  Millisecond accuracy can be offered.
 *
 * @author Marcelo Dalmas <mad@brasmap.com.br> and David Sugar <dyfet@gnutelephony.org>
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
	/**
	 * Create a time from the time portion of a time_t.
	 * @param value of time_t to use.
	 */
	Time(time_t value);

	/**
	 * Create a time from the time portion of a date and time object.
	 * @param object from DateTime::glt() or gmt().
	 */
	Time(tm *dt);

	/**
	 * Create a time from a hh:mm:ss formatted time string.
	 * @param pointer to formatted time field.
	 * @param size of field if not null terminated.
	 */
	Time(char *pointer, size_t size = 0);

	/**
	 * Create a time from hours (0-23), minutes (0-59), and seconds (0-59).
	 * @param hour of time.	
	 * @param minute of time.
	 * @param second of time.
	 */
	Time(int hour, int minute, int second);

	/**
	 * Create a time from current time.
	 */
	Time();

	/**
	 * Destroy time object.
	 */
	virtual ~Time();

	/**
	 * Get current time in seconds from midnight.
	 * @return seconds from midnight.
	 */
	long get(void) const;
	
	/**
	 * Get hours from midnight.
	 * @return hours from midnight.
	 */
	int getHour(void) const;

	/**
	 * Get minutes from current hour.
	 * @return minutes from current hour.
	 */
	int getMinute(void) const;

	/**
	 * Get seconds from current minute.
	 * @return seconds from current minute.
	 */
	int getSecond(void) const;

	/**
	 * Get a hh:mm:ss formatted string for current time.
	 * @param buffer to store time string in.
	 * @return time string buffer or NULL if invalid.
	 */
	char *get(char *buffer) const;

	/**
	 * Set time from a hh:mm:ss formatted string.
	 * @param pointer to time field.
	 * @param size of field if not null terminated.
	 */
	void set(char *str, size_t size = 0);

	/**
	 * Check if time object had valid value.
	 * @return true if object is valid.
	 */
	bool isValid(void) const;

	operator bool();

	long operator-(const Time &ref);
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
protected:
	void update(void);

public:
	static const long c_day;
	static const long c_hour;
	static const long c_week;

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
	DateTime& operator+=(long value);
	DateTime& operator-=(long value);
	DateTime operator+(long value);
	DateTime operator-(long value);

	DateTime& operator++();
	DateTime& operator--();

	int operator==(const DateTime&);
	int operator!=(const DateTime&);
	int operator<(const DateTime&);
	int operator<=(const DateTime&);
	int operator>(const DateTime&);
	int operator>=(const DateTime&);

	bool operator!() const;
	operator bool() const;

	String format(const char *test) const;

	static struct tm *glt(time_t *now = NULL);
	static struct tm *gmt(time_t *now = NULL);
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
typedef	struct tm	tm_t;

END_NAMESPACE

#endif
