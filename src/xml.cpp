#include <config.h>
#include <ucommon/string.h>
#include <ucommon/misc.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

XMLTree::XMLTree(size_t s, char *name) :
mempager(s), root()
{
	size = s;
	root.setId(name);
}

XMLTree::~XMLTree()
{
	root.remove();
	mempager::purge();
}

XMLTree::xmlnode *XMLTree::search(xmlnode *base, const char *find, const char *text)
{
	linked_pointer<xmlnode> node = node->getFirst();
	xmlnode *leaf;
	
	while(node) {
		leaf = node->getLeaf(find);
		if(leaf && !cpr_stricmp(leaf->getData(), text))
			return leaf;
		node.next();
	}
	return NULL;
} 

bool XMLTree::change(xmlnode *node, const char *text)
{
	if(node->getFirst())
		return false;

	node->setPointer(dup(text));
	return true;
}

void XMLTree::remove(xmlnode *node)
{
	node->remove();
	loaded = 0;
}

XMLTree::xmlnode *XMLTree::add(xmlnode *node, const char *id, const char *text)
{
	caddr_t mp = (caddr_t)alloc(sizeof(xmlnode));
	node = new(mp) xmlnode(node, dup(id));

	if(text)
		node->setPointer(dup(text));
	if(validate(node))
		return node;
	remove(node);
	return NULL;
}

bool XMLTree::load(const char *fn, xmlnode *node)
{
	FILE *fp = fopen(fn, "r");
	char *cp, *ep, *bp;
	caddr_t mp;
	ssize_t len = 0;
	bool rtn = false;
	string buffer(size);
	bool document = false;
	xmlnode *match;

	if(!node)
		node = &root;

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
				if(node->getData() != NULL)
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
				node->setPointer(ep);
				*bp = '<';
				cp = bp;
				continue;
			}
	
			*ep = 0;
			cp = ++ep;

			if(!strncmp(bp, "</", 2)) {
				if(strcmp(bp + 2, node->getId()))
					goto exit;
				if(!validate(node))
					goto exit;
				node = node->getParent();
				continue;
			}		
			++bp;

			if(!document) {
				if(strcmp(node->getId(), bp))
					goto exit;
				document = true;
				continue;
			}

			if(node->getData() != NULL)
				goto exit;

			++bp;

			match = node->getChild(bp);
			if(match) {
				match->setPointer(NULL);
				node = match;
				continue;
			}
			mp = (caddr_t)alloc(sizeof(xmlnode));
			node = new(mp) xmlnode(node, dup(bp));
		}
		buffer = cp;
	}
	if(!node && root.getId())
		rtn = true;
exit:
	if(rtn)
		loaded = getPages();
	else
		root.remove();

	fclose(fp);
	return rtn;
}

bool XMLTree::validate(xmlnode *node)
{
	return true;
}
	
