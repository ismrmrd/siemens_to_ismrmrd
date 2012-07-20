#include "XNode.h"

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <stdlib.h>

#include <sstream>

namespace XProtocol
{

const XNode* getChildNodeByName::operator()(const XNodeParamArray& node) const {
	unsigned int index = static_cast<unsigned int>(std::atoi(level_.c_str()));
	const_cast<XNodeParamArray&>(node).expand_children();
	const XNode* ret = 0;
	if (index < node.children_.size()) {
		ret =  &node.children_[index];
	}

	if (!ret) {
		return 0;
	}

	if (has_sublevels_) {
		ret = boost::apply_visitor(getChildNodeByName(sublevel_), *ret);
	}

	return ret;
}

const XNode* getChildNodeByName::operator()(const XNodeParamMap& node) const {
	const XNode* ret = 0;
	BOOST_FOREACH(XNode const& cnode, node.children_) {
		const std::string& name = boost::apply_visitor(getNodeName(), cnode);
		if (boost::iequals(name,level_)) {
		//if (name.compare(level_) == 0) {
			ret = &cnode;
			break;
		}
	}
	if (!ret) {
		return 0;
	}

	if (has_sublevels_) {
		ret = boost::apply_visitor(getChildNodeByName(sublevel_), *ret);
	}

	return ret;
}

const XNode* getChildNodeByName::operator()(const XNodeParamValue& node) const {
	//Values don't have children
	return 0;
}

const XNode*  getChildNodeByIndex::operator()(const XNodeParamMap& node)
{
	const XNode* ret = 0;
	if (node.children_.size() > index_) {
		ret = &node.children_[index_];
	}
	return ret;
}

const XNode*  getChildNodeByIndex::operator()(const XNodeParamArray& node)
{
	const XNode* ret = 0;
	if (node.children_.size() == 0) { //Children have not yet been expanded
		if (!const_cast<XNodeParamArray&>(node).expand_children()) {
			std::cout << "Failed to expand children" << std::endl;
			return 0;
		}
	}
	if (node.children_.size() > index_) {
		ret = &node.children_[index_];
	}

	return ret;
}

const XNode*  getChildNodeByIndex::operator()(const XNodeParamValue& node)
{
	return 0; //Param values do not have children
}


std::string getXMLString::operator()(const XNodeParamMap& node) const
{
	std::stringstream str;
	str << "<" << node.name_ << ">" << std::endl;
	BOOST_FOREACH(XNode const& cnode, node.children_) {
		str << boost::apply_visitor(getXMLString(), cnode);
	}
	str << "</" << node.name_ << ">" << std::endl;
	return str.str();
}

std::string getXMLString::operator()(const XNodeParamArray& node) const
{
	if (node.children_.size() == 0) { //Children have not yet been expanded
		if (!const_cast<XNodeParamArray&>(node).expand_children()) {
			std::cout << "Failed to expand children" << std::endl;
			return 0;
		}
	}
	std::stringstream str;
	str << "<" << node.name_ << ">" << std::endl;
	BOOST_FOREACH(XNode const& cnode, node.children_) {
		str << boost::apply_visitor(getXMLString(), cnode);
	}
	/*
	for (unsigned int i = 0; i < node.values_.size(); i++) {
		str << "<" << node.name_ << ">" << node.values_[i] << "</" << node.name_ << ">" << std::endl;
	}
	*/
	str << "</" << node.name_ << ">" << std::endl;
	return str.str();
}

std::string getXMLString::operator()(const XNodeParamValue& node) const
{
	std::stringstream str;
	for (unsigned int i = 0; i < node.values_.size(); i++) {
		str << "<" << node.name_ << ">" << node.values_[i] << "</" << node.name_ << ">" << std::endl;
	}
	return str.str();
}


bool setNodeValues :: operator()(XNodeParamMap& node) {
	//std::cout << "Calling param map set values....";
	if (node.children_.size() != val_.children_.size()) {
		std::cout << "Mismatch between number of values and children" << std::endl;
		return false;
	}
	for (unsigned int i = 0; i < node.children_.size(); i++) {
		setNodeValues tmp(val_.children_[i]);
		if (!boost::apply_visitor(tmp, node.children_[i])) {
			return false;
		}
	}
	//std::cout << "done!" << std::endl;
	return true;
}

bool setNodeValues :: operator()(XNodeParamArray& node) {
	node.values_.clear();
	for (unsigned int i = 0; i < val_.children_.size(); i++) {
		node.values_.push_back(val_.children_[i]);
	}
	node.expand_children();
	return true;
}

bool setNodeValues :: operator()(XNodeParamValue& node) {
	//std::cout << "Calling param value set values....";
	for (unsigned int i = 0; i < val_.values_.size(); i++) {
		node.values_.push_back(val_.values_[i]);
	}
	//std::cout << "done!" << std::endl;
	return true;
}

bool XNodeParamArray :: expand_children() {
	if (!children_.size()) {
		for (unsigned int i = 0; i < values_.size(); i++) {
			children_.push_back(default_);
			if (values_[i].values_.size() == 0 && values_[i].children_.size()==0) { //Empty values container, occurs sometimes in the files.
				//std::cout << "Empty container encountered, " << name_ << std::endl;
				continue;
			}
			setNodeValues tmp(values_[i]);
			if (!boost::apply_visitor(tmp, children_[i])) {
				return false;
			}
		}
	}
	return true;
}

std::vector<std::string> getStringValueArray::operator()(const XNodeParamArray& node) const
{
	//std::cout << "Array: " << node.name_ << ", node.values_.size() = " << node.values_.size() << std::endl;
	std::vector<std::string> ret;

	return ret;
}

std::vector<std::string> getStringValueArray::operator()(const XNodeParamMap& node) const
{
	std::vector<std::string> ret;

	return ret;
}

std::vector<std::string> getStringValueArray::operator()(const XNodeParamValue& node) const
{
	std::vector<std::string> ret;
	for (unsigned int i = 0; i < node.values_.size(); i++) {
		std::stringstream ss;
		std::string svalue;
		ss << node.values_[i];
		ss >> svalue;
		ret.push_back(svalue);
	}
	return ret;
}

}
