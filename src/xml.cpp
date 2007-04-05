#include <config.h>
#include <ucommon/string.h>
#include <ucommon/misc.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

xmlnode::xmlnode() : 
OrderedObject(), child()
{
	id = NULL;
	text = NULL;
}

xmlnode::xmlnode(xmlnode *p, const char *tag) :
OrderedObject(&p->child), child()
{
	parent = p;
	id = tag;
	text = NULL;
}

const char *xmlnode::getValue(const char *id)
{
	linked_pointer<xmlnode> node = child.begin();
	if(!id)
		return text;

	while(node) {
		if(!strcmp(node->id, id))
			return node->text;
		node.next();
	}
	return NULL;
}

XMLTree::XMLTree(size_t s) :
mempager(s), root()
{
	size = s;
	updated = false;
}

XMLTree::~XMLTree()
{
	mempager::purge();
}

bool XMLTree::change(xmlnode *node, const char *text)
{
	if(node->child.begin())
		return false;

	updated = true;
	node->text = dup(text);
	return true;
}

void XMLTree::remove(xmlnode *node)
{
	node->delist(&node->parent->child);
	updated = true;
	loaded = 0;
}

xmlnode *XMLTree::add(xmlnode *node, const char *id, const char *text)
{
	caddr_t mp = (caddr_t)alloc(sizeof(xmlnode));
	node = new(mp) xmlnode(node, dup(id));
	updated = true;

	if(text)
		node->text = dup(text);
	if(validate(node))
		return node;
	remove(node);
	return NULL;
}

bool XMLTree::load(const char *fn)
{
	FILE *fp = fopen(fn, "r");
	char *cp, *ep, *bp;
	caddr_t mp;
	xmlnode *node = &root;
	ssize_t len = 0;
	bool rtn = false;
	string buffer(size);

	if(!fp)
		return false;

	buffer = "";

	while(node) {
		cp = buffer.c_mem() + buffer.len();
		if(buffer.len() < size - 5) {
			len = fread(cp, 1, size - buffer.len() - 1, fp);
		}
		else
			len = 0;


		if(len < 0)
			goto exit;
		cp[len] = 0;
		if(!buffer.chr('<'))
			goto exit;
		buffer = buffer.c_str();
		cp = buffer.c_mem();

		while(node && cp && *cp)
		{
			while(isspace(*cp))
				++cp;

			if(cp && *cp && !node)
				goto exit;

			bp = strchr(cp, '<');
			ep = strchr(cp, '>');
			if(!ep && bp == cp)
				break;
			if(!bp ) {
				cp = cp + strlen(cp);
				break;
			}
			if(bp > cp) {
				if(node->text)
					goto exit;
				*bp = 0;
				ep = bp - 1;
				while(ep > cp && isspace(*ep)) {
					*ep = 0;
					--ep;
				}		
				len = strlen(cp);
				ep = (char *)alloc(len);
				cpr_xmldecode(ep, len, cp);
				node->text = ep;
				*bp = '<';
				cp = bp;
				continue;
			}
	
			*ep = 0;
			cp = ++ep;

			if(!strncmp(bp, "</", 2)) {
				if(strcmp(bp + 2, node->id))
					goto exit;
				if(!validate(node))
					goto exit;
				node = node->parent;
				continue;
			}		
			if(!node->id) {
				node->id = dup(++bp);
				continue;
			}
			if(node->text)
				goto exit;
			mp = (caddr_t)alloc(sizeof(xmlnode));
			node = new(mp) xmlnode(node, dup(++bp));
		}
		buffer = cp;
	}
	if(!node && root.id)
		rtn = true;
exit:
	if(rtn)
		loaded = getPages();

	fclose(fp);
	return rtn;
}

bool XMLTree::validate(xmlnode *node)
{
	return true;
}
	
