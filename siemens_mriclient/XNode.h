#ifndef XNODE_H
#define XNODE_H

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace XProtocol
{
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef
		boost::variant<
		std::string
		, long
		, double
		> XNodeValueVariant;

struct XNodeParamValue
{
	std::string name_;
	std::string type_;
	std::vector<XNodeValueVariant> values_;
};

struct XNodeArrayValue
{
	std::vector<XNodeValueVariant> values_;
	std::vector<XNodeArrayValue> children_;
};

struct XNodeParamMap;
struct XNodeParamArray;

typedef
		boost::variant<
		boost::recursive_wrapper<XNodeParamMap>
, boost::recursive_wrapper<XNodeParamArray>
, XNodeParamValue
>
XNode;


struct XNodeParamMap
{
	std::string name_;
	std::string type_;
	std::vector<XNode> children_;
};

struct XNodeParamArray
{
	std::string name_;
	std::string type_;
	XNode default_;
	std::vector<XNodeArrayValue> values_;
	std::vector<XNode> children_;

	bool expand_children();
};

int ParseXProtocol(std::string& input, XNode& tree);

class setNodeValues : public boost::static_visitor<bool> {
public:
	setNodeValues(const XNodeArrayValue val)
	: val_(val)
	{

	}

	bool operator()(XNodeParamMap& node);
	bool operator()(XNodeParamArray& node);
	bool operator()(XNodeParamValue& node);

protected:
	XNodeArrayValue val_;
};

class getNodeName : public boost::static_visitor<std::string> {
public:
	template <typename T> const std::string& operator()(const T& node) const
	{
		return node.name_;
	}
};

class getChildNodeByIndex : public boost::static_visitor<const XNode*> {
public:
	getChildNodeByIndex(unsigned int idx)
	: index_(idx)
	{

	}

	const XNode* operator()(const XNodeParamMap& node);
	const XNode* operator()(const XNodeParamArray& node);
	const XNode* operator()(const XNodeParamValue& node);

protected:
	unsigned int index_;
};

class getChildNodeByName : public boost::static_visitor<const XNode*> {
public:
	getChildNodeByName(std::string name)
	: name_(name)
	, level_("")
	, sublevel_("")
	, has_sublevels_(false)
	{
		std::vector<std::string> split_path;
		boost::split( split_path, name_, boost::is_any_of("."), boost::token_compress_on );
		level_ = split_path[0];
		if (split_path.size() > 1) {
			has_sublevels_ = true;
			sublevel_ = split_path[1];
			for (unsigned int i = 2; i < split_path.size(); i++) {
				sublevel_ += std::string(".") + split_path[i];
			}
		}
	}

	const XNode* operator()(const XNodeParamArray& node) const;
	const XNode* operator()(const XNodeParamMap& node) const;
	const XNode* operator()(const XNodeParamValue& node) const;

protected:
	std::string name_;
	std::string level_;
	std::string sublevel_;
	bool has_sublevels_;
};


class getTypeName : public boost::static_visitor<std::string> {
public:
	std::string operator()(const XNodeParamMap& node) const {
		return std::string("ParamMap");
	}

	std::string operator()(const XNodeParamArray& node) const {
		return std::string("ParamArray");
	}

	std::string operator()(const XNodeParamValue& node) const {
		return std::string("ParamValue");
	}

};

class getStringValueArray : public boost::static_visitor< std::vector<std::string> > {
public:
	std::vector<std::string> operator()(const XNodeParamArray& node) const;
	std::vector<std::string> operator()(const XNodeParamMap& node) const;
	std::vector<std::string> operator()(const XNodeParamValue& node) const;
};

class getXMLString : public boost::static_visitor<std::string> {
public:
	std::string operator()(const XNodeParamMap& node) const;
	std::string operator()(const XNodeParamArray& node) const;
	std::string operator()(const XNodeParamValue& node) const;
};


}

BOOST_FUSION_ADAPT_STRUCT(
		XProtocol::XNodeParamMap,
		(std::string, name_)
		(std::string, type_)
		(std::vector<XProtocol::XNode>, children_)
)

BOOST_FUSION_ADAPT_STRUCT(
		XProtocol::XNodeParamArray,
		(std::string, name_)
		(std::string, type_)
		(XProtocol::XNode, default_)
		(std::vector<XProtocol::XNodeArrayValue>, values_)
		(std::vector<XProtocol::XNode>, children_)
)

BOOST_FUSION_ADAPT_STRUCT(
		XProtocol::XNodeArrayValue,
		(std::vector<XProtocol::XNodeValueVariant>, values_)
		(std::vector<XProtocol::XNodeArrayValue>, children_)
)

BOOST_FUSION_ADAPT_STRUCT(
		XProtocol::XNodeParamValue,
		(std::string, name_)
		(std::string, type_)
		(std::vector<XProtocol::XNodeValueVariant>, values_)
)

#endif //XNODE_H
