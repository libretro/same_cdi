// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    xmlfile.c

    XML file parsing code.

***************************************************************************/

#include "xmlfile.h"

#include "osdcore.h"

#include "formats/rxml.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>


namespace util::xml {

namespace {

//**************************************************************************
//  CONSTANTS
//**************************************************************************

constexpr unsigned TEMP_BUFFER_SIZE(4096U);


//**************************************************************************
//  UTILITY FUNCTIONS
//**************************************************************************

void write_escaped(core_file &file, std::string const &str)
{
	std::string::size_type pos = 0;
	while ((str.size() > pos) && (std::string::npos != pos))
	{
		std::string::size_type const found = str.find_first_of("\"&<>", pos);
		if (found != std::string::npos)
		{
			std::size_t written;
			file.write(&str[pos], found - pos, written);
			switch (str[found])
			{
			case '"': file.puts("&quot;"); pos = found + 1; break;
			case '&': file.puts("&amp;");  pos = found + 1; break;
			case '<': file.puts("&lt;");   pos = found + 1; break;
			case '>': file.puts("&gt;");   pos = found + 1; break;
			default: pos = found;
			}
		}
		else
		{
			std::size_t written;
			file.write(&str[pos], str.size() - pos, written);
			pos = found;
		}
	}
}

} // anonymous namespace



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

//**************************************************************************
//  RXML BRIDGE
//**************************************************************************

// Build the util::xml::data_node tree from a parsed rxml document.
// Replicates what the expat callbacks used to do: every element becomes
// a child node carrying its source line and attributes in document
// order; element text becomes the node's value, trimmed of leading and
// trailing whitespace as before.  Semantic deltas versus expat, none of
// which the XML dialects in this tree (layouts, cfg, cheats, software
// lists) exercise, are documented in the commit that made the switch:
// whitespace-only runs inside mixed content are dropped rather than
// preserved between the non-whitespace fragments, and input is
// required to be UTF-8 (declared legacy encodings are not transcoded).
static bool rxml_to_data_node(data_node *parent, ::rxml_node_t *src)
{
	for ( ; src; src = src->next)
	{
		data_node *const node = parent->add_child(src->name, nullptr);
		if (!node)
			return false;
		node->line = int(src->line);
		for (::rxml_attrib_node *at = src->attrib; at; at = at->next)
			node->add_attribute(at->attrib, at->value ? at->value : "");
		if (src->data)
		{
			node->append_value(src->data, int(strlen(src->data)));
			node->trim_whitespace();
		}
		if (src->children && !rxml_to_data_node(node, src->children))
			return false;
	}
	return true;
}

static file::ptr parse_string(char const *string, parse_options const *opts)
{
	::rxml_parse_error_t rerr;
	::rxml_document_t *const doc = ::rxml_load_document_string_opts(
			string, RXML_OPT_STRICT_EOF | RXML_OPT_LINES, &rerr);
	if (!doc)
	{
		if (opts && opts->error)
		{
			opts->error->error_message = "malformed XML";
			opts->error->error_line = int(rerr.line);
			opts->error->error_column = int(rerr.col);
		}
		return file::ptr();
	}

	file::ptr result = file::create();
	if (result && !rxml_to_data_node(result.get(), ::rxml_root_node(doc)))
		result.reset();
	::rxml_free_document(doc);
	return result;
}



//**************************************************************************
//  XML FILE OBJECTS
//**************************************************************************

file::file() { }
file::~file() { }


//-------------------------------------------------
//  create - create a new, empty XML file
//-------------------------------------------------

file::ptr file::create()
{
	try { return ptr(new file); }
	catch (...) { return ptr(); }
}


//-------------------------------------------------
//  read - parse an XML file into its nodes
//-------------------------------------------------

file::ptr file::read(read_stream &file, parse_options const *opts)
{
	// slurp the stream; rxml parses from a single in-memory document
	std::string buffer;
	for (;;)
	{
		char tempbuf[TEMP_BUFFER_SIZE];
		size_t bytes;
		file.read(tempbuf, sizeof(tempbuf), bytes); // TODO: better error handling
		if (!bytes)
			break;
		buffer.append(tempbuf, bytes);
	}
	return parse_string(buffer.c_str(), opts);
}


//-------------------------------------------------
//  string_read - parse an XML string into its
//  nodes
//-------------------------------------------------

file::ptr file::string_read(const char *string, parse_options const *opts)
{
	return parse_string(string, opts);
}


//-------------------------------------------------
//  file_write - write an XML tree to a file
//-------------------------------------------------

void file::write(util::core_file &file) const
{
	// ensure this is a root node
	assert(!get_name());

	// output a simple header
	file.printf("<?xml version=\"1.0\"?>\n");
	file.printf("<!-- This file is autogenerated; comments and unknown tags will be stripped -->\n");

	// loop over children of the root and output
	write_recursive(0, file);
}



//**************************************************************************
//  XML NODE MANAGEMENT
//**************************************************************************

data_node::data_node()
	: line(0)
	, m_next(nullptr)
	, m_first_child(nullptr)
	, m_name()
	, m_value()
	, m_parent(nullptr)
	, m_attributes()
{
}

data_node::data_node(data_node *parent, const char *name, const char *value)
	: line(0)
	, m_next(nullptr)
	, m_first_child(nullptr)
	, m_name(name)
	, m_value(value ? value : "")
	, m_parent(parent)
	, m_attributes()
{
	std::transform(m_name.begin(), m_name.end(), m_name.begin(), [] (char ch) { return std::tolower(uint8_t(ch)); });
}


//-------------------------------------------------
//  free_node_recursive - recursively free
//  the data allocated to an XML node
//-------------------------------------------------

data_node::~data_node()
{
	free_children();
}


void data_node::free_children()
{
	for (data_node *nchild = nullptr; m_first_child; m_first_child = nchild)
	{
		// note the next node and free this node
		nchild = m_first_child->get_next_sibling();
		delete m_first_child;
	}
}


void data_node::set_value(char const *value)
{
	m_value.assign(value ? value : "");
}


void data_node::append_value(char const *value, int length)
{
	m_value.append(value, length);
}


void data_node::trim_whitespace()
{
	// first strip leading spaces
	std::string::iterator start = m_value.begin();
	while ((m_value.end() != start) && std::isspace(uint8_t(*start)))
		++start;
	m_value.replace(m_value.begin(), start, 0U, '\0');

	// then strip trailing spaces
	std::string::iterator end = m_value.end();
	while ((m_value.begin() != end) && std::isspace(uint8_t(*std::prev(end))))
		--end;
	m_value.replace(end, m_value.end(), 0U, '\0');
}


//-------------------------------------------------
//  data_node::count_children - count the
//  number of child nodes
//-------------------------------------------------

std::size_t data_node::count_children() const
{
	// loop over children and count
	std::size_t count = 0;
	for (data_node const *node = get_first_child(); node; node = node->get_next_sibling())
		count++;
	return count;
}


//-------------------------------------------------
//  data_node::count_children - count the
//  number of child nodes
//-------------------------------------------------

std::size_t data_node::count_attributes() const
{
	return m_attributes.size();
}


//-------------------------------------------------
//  data_node::get_child - find the first
//  child of the specified node with the specified
//  tag
//-------------------------------------------------

data_node *data_node::get_child(const char *name)
{
	return m_first_child ? m_first_child->get_sibling(name) : nullptr;
}

data_node const *data_node::get_child(const char *name) const
{
	return m_first_child ? m_first_child->get_sibling(name) : nullptr;
}


//-------------------------------------------------
//  find_first_matching_child - find the first
//  child of the specified node with the
//  specified tag or attribute/value pair
//-------------------------------------------------

data_node *data_node::find_first_matching_child(const char *name, const char *attribute, const char *matchval)
{
	return m_first_child ? m_first_child->find_matching_sibling(name, attribute, matchval) : nullptr;
}

data_node const *data_node::find_first_matching_child(const char *name, const char *attribute, const char *matchval) const
{
	return m_first_child ? m_first_child->find_matching_sibling(name, attribute, matchval) : nullptr;
}


//-------------------------------------------------
//  data_node::get_next_sibling - find the
//  next sibling of the specified node with the
//  specified tag
//-------------------------------------------------

data_node *data_node::get_next_sibling(const char *name)
{
	return m_next ? m_next->get_sibling(name) : nullptr;
}

data_node const *data_node::get_next_sibling(const char *name) const
{
	return m_next ? m_next->get_sibling(name) : nullptr;
}


//-------------------------------------------------
//  find_next_matching_sibling - find the next
//  sibling of the specified node with the
//  specified tag or attribute/value pair
//-------------------------------------------------

data_node *data_node::find_next_matching_sibling(const char *name, const char *attribute, const char *matchval)
{
	return m_next ? m_next->find_matching_sibling(name, attribute, matchval) : nullptr;
}

data_node const *data_node::find_next_matching_sibling(const char *name, const char *attribute, const char *matchval) const
{
	return m_next ? m_next->find_matching_sibling(name, attribute, matchval) : nullptr;
}


//-------------------------------------------------
//  add_child - add a new child node to the
//  given node
//-------------------------------------------------

data_node *data_node::add_child(const char *name, const char *value)
{
	if (!name || !*name)
		return nullptr;

	// new element: create a new node
	data_node *node;
	try { node = new data_node(this, name, value); }
	catch (...) { return nullptr; }

	if (!node->get_name() || (!node->get_value() && value))
	{
		delete node;
		return nullptr;
	}

	// add us to the end of the list of siblings
	data_node **pnode;
	for (pnode = &m_first_child; *pnode; pnode = &(*pnode)->m_next) { }
	*pnode = node;

	return node;
}


//-------------------------------------------------
//  get_or_add_child - find a child node of
//  the specified type; if not found, add one
//-------------------------------------------------

data_node *data_node::get_or_add_child(const char *name, const char *value)
{
	// find the child first
	data_node *const child = m_first_child->get_sibling(name);
	if (child)
		return child;

	// if not found, do a standard add child
	return add_child(name, value);
}


// recursively copy as child of another node
data_node *data_node::copy_into(data_node &parent) const
{
	data_node *const result = parent.add_child(get_name(), get_value());
	result->m_attributes = m_attributes;

	data_node *dst = result;
	data_node const *src = get_first_child();
	while (src && (&parent != dst))
	{
		dst = dst->add_child(src->get_name(), src->get_value());
		dst->m_attributes = src->m_attributes;
		data_node const *next = src->get_first_child();
		if (next)
		{
			src = next;
		}
		else
		{
			dst = dst->get_parent();
			next = src->get_next_sibling();
			if (next)
			{
				src = next;
			}
			else
			{
				dst = dst->get_parent();
				src = src->get_parent()->get_next_sibling();
			}
		}
	}

	return result;
}


//-------------------------------------------------
//  delete_node - delete a node and its
//  children
//-------------------------------------------------

void data_node::delete_node()
{
	// don't allow deletion of document root
	if (m_parent)
	{
		// first unhook us from the list of children of our parent
		for (data_node **pnode = &m_parent->m_first_child; *pnode; pnode = &(*pnode)->m_next)
		{
			if (*pnode == this)
			{
				*pnode = this->m_next;
				break;
			}
		}

		// now free ourselves and our children
		delete this;
	}
	else
	{
		// remove all children of document root
		free_children();
	}
}


//-------------------------------------------------
//  get_next_sibling - find the next sibling of
//  the specified node with the specified tag
//-------------------------------------------------

data_node *data_node::get_sibling(const char *name)
{
	// loop over siblings and find a matching name
	for (data_node *node = this; node; node = node->get_next_sibling())
		if (strcmp(node->get_name(), name) == 0)
			return node;
	return nullptr;
}

data_node const *data_node::get_sibling(const char *name) const
{
	// loop over siblings and find a matching name
	for (data_node const *node = this; node; node = node->get_next_sibling())
		if (strcmp(node->get_name(), name) == 0)
			return node;
	return nullptr;
}


//-------------------------------------------------
//  find_matching_sibling - find the next
//  sibling of the specified node with the
//  specified tag or attribute/value pair
//-------------------------------------------------

data_node *data_node::find_matching_sibling(const char *name, const char *attribute, const char *matchval)
{
	// loop over siblings and find a matching attribute
	for (data_node *node = this; node; node = node->get_next_sibling())
	{
		// can pass nullptr as a wildcard for the node name
		if (!name || !strcmp(name, node->get_name()))
		{
			// find a matching attribute
			attribute_node const *const attr = node->get_attribute(attribute);
			if (attr && !strcmp(attr->value.c_str(), matchval))
				return node;
		}
	}
	return nullptr;
}

data_node const *data_node::find_matching_sibling(const char *name, const char *attribute, const char *matchval) const
{
	// loop over siblings and find a matching attribute
	for (data_node const *node = this; node; node = node->get_next_sibling())
	{
		// can pass nullptr as a wildcard for the node name
		if (!name || !strcmp(name, node->get_name()))
		{
			// find a matching attribute
			attribute_node const *const attr = node->get_attribute(attribute);
			if (attr && !strcmp(attr->value.c_str(), matchval))
				return node;
		}
	}
	return nullptr;
}



//**************************************************************************
//  XML ATTRIBUTE MANAGEMENT
//**************************************************************************

bool data_node::has_attribute(const char *attribute) const
{
	return get_attribute(attribute) != nullptr;
}


//-------------------------------------------------
//  get_attribute - get the value of the
//  specified attribute, or nullptr if not found
//-------------------------------------------------

data_node::attribute_node *data_node::get_attribute(const char *attribute)
{
	// loop over attributes and find a match
	for (attribute_node &anode : m_attributes)
		if (strcmp(anode.name.c_str(), attribute) == 0)
			return &anode;
	return nullptr;
}

data_node::attribute_node const *data_node::get_attribute(const char *attribute) const
{
	// loop over attributes and find a match
	for (attribute_node const &anode : m_attributes)
		if (strcmp(anode.name.c_str(), attribute) == 0)
			return &anode;
	return nullptr;
}


//-------------------------------------------------
//  get_attribute_string_ptr - get a pointer to
//  the string value of the specified attribute;
//  if not found, return = nullptr
//-------------------------------------------------

std::string const *data_node::get_attribute_string_ptr(const char *attribute) const
{
	attribute_node const *attr = get_attribute(attribute);
	return attr ? &attr->value : nullptr;
}


//-------------------------------------------------
//  get_attribute_string - get the string
//  value of the specified attribute; if not
//  found, return = the provided default
//-------------------------------------------------

const char *data_node::get_attribute_string(const char *attribute, const char *defvalue) const
{
	attribute_node const *attr = get_attribute(attribute);
	return attr ? attr->value.c_str() : defvalue;
}


//-------------------------------------------------
//  get_attribute_int - get the integer
//  value of the specified attribute; if not
//  found, return = the provided default
//-------------------------------------------------

long long data_node::get_attribute_int(const char *attribute, long long defvalue) const
{
	attribute_node const *attr = get_attribute(attribute);
	if (!attr)
		return defvalue;
	std::string const &string = attr->value;

	std::istringstream stream;
	stream.imbue(std::locale::classic());
	long long result;
	if (string[0] == '$')
	{
		stream.str(&string[1]);
		unsigned long long uvalue;
		stream >> std::hex >> uvalue;
		result = static_cast<long long>(uvalue);
	}
	else if ((string[0] == '0') && ((string[1] == 'x') || (string[1] == 'X')))
	{
		stream.str(&string[2]);
		unsigned long long uvalue;
		stream >> std::hex >> uvalue;
		result = static_cast<long long>(uvalue);
	}
	else if (string[0] == '#')
	{
		stream.str(&string[1]);
		stream >> result;
	}
	else
	{
		stream.str(&string[0]);
		stream >> result;
	}

	return stream ? result : defvalue;
}


//-------------------------------------------------
//  get_attribute_int_format - return the
//  format of the given integer attribute
//-------------------------------------------------

data_node::int_format data_node::get_attribute_int_format(const char *attribute) const
{
	attribute_node const *attr = get_attribute(attribute);
	if (!attr)
		return int_format::DECIMAL;
	std::string const &string = attr->value;

	if (string[0] == '$')
		return int_format::HEX_DOLLAR;
	else if (string[0] == '0' && string[1] == 'x')
		return int_format::HEX_C;
	else if (string[0] == '#')
		return int_format::DECIMAL_HASH;
	else
		return int_format::DECIMAL;
}


//-------------------------------------------------
//  get_attribute_float - get the float
//  value of the specified attribute; if not
//  found, return = the provided default
//-------------------------------------------------

float data_node::get_attribute_float(const char *attribute, float defvalue) const
{
	attribute_node const *attr = get_attribute(attribute);
	if (!attr)
		return defvalue;

	std::istringstream stream(attr->value);
	stream.imbue(std::locale::classic());
	float result;
	return (stream >> result) ? result : defvalue;
}


//-------------------------------------------------
//  set_attribute - set a new attribute and
//  string value on the node
//-------------------------------------------------

void data_node::set_attribute(const char *name, const char *value)
{
	attribute_node *anode;

	// first find an existing one to replace
	anode = get_attribute(name);

	if (anode != nullptr)
	{
		// if we found it, free the old value and replace it
		anode->value = value;
	}
	else
	{
		// otherwise, create a new node
		add_attribute(name, value);
	}
}


//-------------------------------------------------
//  set_attribute_int - set a new attribute and
//  integer value on the node
//-------------------------------------------------

void data_node::set_attribute_int(const char *name, long long value)
{
	set_attribute(name, string_format(std::locale::classic(), "%d", value).c_str());
}


//-------------------------------------------------
//  set_attribute_int - set a new attribute and
//  float value on the node
//-------------------------------------------------

void data_node::set_attribute_float(const char *name, float value)
{
	set_attribute(name, string_format(std::locale::classic(), "%f", value).c_str());
}



//**************************************************************************
//  MISCELLANEOUS INTERFACES
//**************************************************************************

//-------------------------------------------------
//  normalize_string - normalize a string
//  to ensure it doesn't contain embedded tags
//-------------------------------------------------

const char *normalize_string(const char *string)
{
	static char buffer[1024];
	char *d = &buffer[0];

	if (string != nullptr)
	{
		while (*string)
		{
			switch (*string)
			{
				case '\"' : d += sprintf(d, "&quot;"); break;
				case '&'  : d += sprintf(d, "&amp;"); break;
				case '<'  : d += sprintf(d, "&lt;"); break;
				case '>'  : d += sprintf(d, "&gt;"); break;
				default:
					*d++ = *string;
			}
			++string;
		}
	}
	*d++ = 0;
	return buffer;
}



//**************************************************************************
//  EXPAT INTERFACES
//**************************************************************************
//  NODE/ATTRIBUTE ADDITIONS
//**************************************************************************

//-------------------------------------------------
//  add_attribute - add a new attribute to the
//  given node
//-------------------------------------------------

void data_node::add_attribute(const char *name, const char *value)
{
	try
	{
		attribute_node &anode = *m_attributes.emplace(m_attributes.end(), name, value);
		std::transform(anode.name.begin(), anode.name.end(), anode.name.begin(), [] (char ch) { return std::tolower(uint8_t(ch)); });
	}
	catch (...)
	{
		osd_printf_warning("error adding attribute to XML data node\n");
	}
}



//**************************************************************************
//  RECURSIVE TREE OPERATIONS
//**************************************************************************

//-------------------------------------------------
//  write_node_recursive - recursively write
//  an XML node and its children to a file
//-------------------------------------------------

void data_node::write_recursive(int indent, util::core_file &file) const
{
	if (!get_name())
	{
		// root node doesn't generate tag
		for (data_node const *child = this->get_first_child(); child; child = child->get_next_sibling())
			child->write_recursive(indent, file);
	}
	else
	{
		// output this tag
		file.printf("%*s<%s", indent, "", get_name());

		// output any attributes, escaping as necessary
		for (attribute_node const &anode : m_attributes)
		{
			file.printf(" %s=\"", anode.name);
			write_escaped(file, anode.value);
			file.puts("\"");
		}

		if (!get_first_child() && !get_value())
		{
			// if there are no children and no value, end the tag here
			file.printf(" />\n");
		}
		else
		{
			// otherwise, close this tag and output more stuff
			file.printf(">\n");

			// if there is a value, output that here
			if (!m_value.empty())
			{
				file.printf("%*s", indent + 4, "");
				write_escaped(file, m_value);
				file.puts("\n");
			}

			// loop over children and output them as well
			if (get_first_child())
			{
				for (data_node const *child = this->get_first_child(); child; child = child->get_next_sibling())
					child->write_recursive(indent + 4, file);
			}

			// write a closing tag
			file.printf("%*s</%s>\n", indent, "", get_name());
		}
	}
}

} // namespace util::xml
