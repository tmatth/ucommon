// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

/**
 * Parsing of config files that have keyword/value pairs.  This includes
 * supporting classes to extract basic config data from files that are stored
 * as []'s, and uses several supporting classes.
 * @file ucommon/keydata.h
 */

#ifndef	_UCOMMON_KEYDATA_H_
#define	_UCOMMON_KEYDATA_H_

#ifndef	 _UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	 _UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

NAMESPACE_UCOMMON

class keyfile;

/**
 * Data keys parsed from a keyfile.  This is a specific [] section from a
 * fully loaded keyfile, and offers common means to access data members.
 * This is related to the original GNU Common C++ keydata object, although
 * it is formed in a keyfile class which is loaded from a config file all
 * at once.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT keydata : public OrderedObject
{
private:

	friend class keyfile;
	OrderedIndex index;
	keydata();
	keydata(keyfile *file, const char *id);
	const char *name;

public:
	/**
	 * A key value set is used for iterative access.  Otherwise this class
	 * is normally not used as we usually request the keys directly.
	 * @author David Sugar <dyfet@gnutelephony.org>
	 */
	class __LOCAL keyvalue : public OrderedObject
	{
	private:
		friend class keydata;
		friend class keyfile;
		keyvalue(keyfile *allocator, keydata *section, const char *key, const char *data);
	public:
		const char *id;
		const char *value;
	};

	/**
	 * Lookup a key value by it's id.
	 * @param id to look for.
	 * @return value string or NULL if not found.
	 */
	const char *get(const char *id);

	/**
	 * Lookup a key value by it's id.
	 * @param id to look for.
	 * @return value string or NULL if not found.
	 */
	inline const char *operator()(const char *id)
		{return get(id);};

	/**
	 * Get the name of this section.  Useful in iterative examinations.
	 * @return name of keydata section.
	 */
	inline const char *get(void)
		{return name;};

	/**
	 * Get first value object, for iterative examinations.
	 * @return first key value in chain.
	 */
	inline keyvalue *begin(void)
		{return (keyvalue *)index.begin();};

	/**
	 * Get last value object, for iterative examinations.
	 * @return first key value in chain.
	 */
	inline keyvalue *end(void)
		{return (keyvalue*)index.end();};

	/**
	 * Convenience typedef for iterative pointer.
	 */
	typedef linked_pointer<keyvalue> iterator;
};	

/**
 * Traditional keypair config file parsing class.  This is used to get
 * generic config data either from a /etc/xxx.conf, a windows style
 * xxx.ini file, or a ~/.xxxrc file, and parses [] sections from the 
 * entire file at once.
 */
class __EXPORT keyfile : public mempager
{
private:
	friend class keydata;
	OrderedIndex index;
	keydata *defaults;

	keydata *create(const char *section);
	void create(keydata *keydata, const char *id, const char *value);

public:
	/**
	 * Create an empty key file ready for loading.
	 * @param pagesize for memory paging.
	 */
	keyfile(size_t pagesize = 0);

	/**
	 * Create a key file object from an existing config file.
	 * @param path to load from.
	 * @param pagesize for memory paging.
	 */
	keyfile(const char *path, size_t pagesize = 0);

	/**
	 * Load (overlay) another config file over the currently loaded one.
	 * This is used to merge key data, such as getting default values from
	 * a global config, and then overlaying a local home config file.
	 * @param path to load keys from into current object.
	 */
	void load(const char *path);

	/**
	 * Get a keydata section name.
	 * @param section name to look for.
	 * @return keydata section object if found, NULL if not.
	 */
	keydata *get(const char *section);

	inline keydata *operator()(const char *section)
		{return get(section);};

	inline keydata *operator[](const char *section)
		{return get(section);};

	/**
	 * Get the non-sectioned defaults if there are any.
	 * @return default key section.
	 */
	inline keydata *get(void)
		{return defaults;};

	/**
	 * Get first keydata object, for iterative examinations.
	 * @return first key value in chain.
	 */
	inline keydata *begin(void)
		{return (keydata *)index.begin();};

	/**
	 * Get last keydata object, for iterative examinations.
	 * @return first key value in chain.
	 */
	inline keydata *end(void)
		{return (keydata *)index.end();};
	
	/**
	 * Convenience typedef for iterative pointer.
	 */
	typedef linked_pointer<keydata> iterator;
};

END_NAMESPACE

#endif
