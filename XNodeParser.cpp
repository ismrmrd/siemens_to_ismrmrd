#include "XNode.h"

#include <iostream>

#include <boost/bind.hpp>

namespace XProtocol
{


template <typename Iterator>
struct XNodeGrammar : qi::grammar<Iterator, XNodeParamMap(), ascii::space_type>
{
	XNodeGrammar() : XNodeGrammar::base_type(xprot)
	{
		using qi::lit;
		using qi::lexeme;
		using ascii::char_;
		using ascii::string;
		using namespace qi::labels;
		using boost::spirit::long_;
		using boost::spirit::double_;
		using boost::spirit::qi::real_parser;
		using boost::spirit::qi::strict_real_policies;
		using boost::spirit::_1;

		using phoenix::at_c;
		using phoenix::push_back;
		
		node = (param_map | param_array | param_generic)       [_val = _1];

		burn_properties =
				(  (lit("<Default>") >> double_)
						| (lit("<Default>") >> long_)
						| (lit("<Default>") >> quoted_string)
						| (lit("<Precision>") >> long_)
						| (lit("<MinSize>") >> long_)
						| (lit("<MaxSize>") >> long_)
						| (lit("<Comment>") >> quoted_string)
						| (lit("<Visible>") >> quoted_string)
						| (lit("<Tooltip>") >> quoted_string)
						| (lit("<Class>") >> quoted_string)
						| (lit("<Label>") >> quoted_string)
						//| (lit("<Comment>") >> quoted_string) @malte twice ?
						| (lit("<Unit>") >> quoted_string)
						| (lit("<InFile>") >> quoted_string)
						| (lit("<Dll>") >> quoted_string)
						| (lit("<Repr>") >> quoted_string)
						| (lit("<LimitRange>") >> '{' >> *(quoted_string | long_ | double_) >> '}' )
						| (lit("<Limit>") >> '{' >> *(quoted_string | long_ | double_) >> '}' )
				) //Burn this
				;

		burn_param_card_layout =
//				lit("<ParamCardLayout.\"Inline Evaluation\">")
				lit("<ParamCardLayout.") >> quoted_string >> '>'
				>> '{'
				>> lit("<Repr>") >> quoted_string
				>> *(lit("<Control>  {") >> lexeme[+(char_ - '}')] >> '}')
				>> *(lit("<Line>  {") >> lexeme[+(char_ - '}')] >> '}')
				>> '}'
				;

		burn_dependency =
				((lit("<Dependency.") | lit("<ProtocolComposer.")) >> quoted_string >> '>'
						>> '{'
						>>  lexeme[+(char_ - '}')] >> '}')
						;

		quoted_string =
				'"' >> lexeme[*(char_ - '"') [_val += _1]] >> '"';
		
		param_generic =
				'<'
				>> lexeme[+(char_ - '.')[at_c<1>(_val) += _1]] >> '.'
//		  >> quoted_string[std::cout << "GENERIC: " << at_c<1>(_val) << ", " << (at_c<0>(_val) = _1) << std::endl]
				>> quoted_string[at_c<0>(_val) = _1]
				                 >> '>' >> '{'
				                 >> *burn_properties
				                 >> *((quoted_string[push_back(at_c<2>(_val),_1)])| (strict_double[push_back(at_c<2>(_val),_1)]) | (long_[push_back(at_c<2>(_val),_1)]) | (lit("<Line>  {") >> *(char_ -'}') >> '}'))
				                 >> '}'
				                 ;

		array_value =
				'{'
				>> *burn_properties
				>> ((*array_value[push_back(at_c<1>(_val),_1)] >> '}') |
						(*quoted_string[push_back(at_c<0>(_val), _1)] >> '}') |
						(*strict_double[push_back(at_c<0>(_val),_1 )] >> '}' ) |
						(*double_[push_back(at_c<0>(_val),_1 )] >> '}' ) |  //@Malte
						(*long_[push_back(at_c<0>(_val),_1 )] >> '}' ))
						;

		param_array =
				lit("<ParamArray.")[at_c<1>(_val) = std::string("ParamArray")]
				                    >>  quoted_string[at_c<0>(_val) = _1]
//				                    >> quoted_string[std::cout << "ARRAY: " << at_c<1>(_val) << ", " << _1 << std::endl]
				                                      >> '>' >> '{'
#ifndef NUMARIS_NX	
													  >> *((lit("<Visible>") >> quoted_string) | (lit("<MinSize>") >> long_) | (lit("<Label>") >> quoted_string) | (lit("<MaxSize>") >> long_) | (lit("<Comment>") >> quoted_string)) //Burn this
#else
				                                      >> *((lit("<Visible>") >> quoted_string) | (lit("<DefaultSize>") >> long_) | (lit("<Label>") >> quoted_string) | (lit("<MaxSize>") >> long_) | (lit("<Comment>") >> quoted_string)) //@Malte DefaultSize was MinSize//Burn this
#endif				                                      
													  >> lit("<Default>")
				                                      >> node[at_c<2>(_val)  = _1]
				                                              >> *array_value[push_back(at_c<3>(_val),_1)]
				                                                              >> '}'
				                                                              ;

		param_map =
				(lit("<ParamMap.\"")[at_c<1>(_val) = std::string("ParamMap")] |
						lit("<Pipe.\"")[at_c<1>(_val) = std::string("Pipe")] |
						lit("<PipeService.\"")[at_c<1>(_val) = std::string("PipeService")]  |
						lit("<ParamFunctor.\"")[at_c<1>(_val) = std::string("ParamFunctor")] )
						>> lexeme[*(char_ - '"')       [at_c<0>(_val) += _1]]
						          >> '"' >> '>' >> '{'
						          >> *burn_properties
						          >> +node [push_back(at_c<2>(_val), _1)]
						                    >> '}'
						                    ;
		
		xprot =
				lit("<XProtocol>")[at_c<1>(_val) = std::string("XProtocol")]
				                   >> '{'
				                   >> * (lit("<Name>") >> quoted_string)                                   //Burn this
				                   >> * (lit("<ID>") >> long_)                                            //Burn this
#ifndef NUMARIS_NX				                   
								   >> * (lit("<Userversion>") >> lexeme[*(char_ - '\n')])                //Burn this
#else
									>> * (lit("<Userversion>") >> long_) 
#endif
				                   >> * (lit("<EVAStringTable>") >> '{' >> lexeme[+(char_ - '}')] >> '}') //Burn this
				                   >> +node [push_back(at_c<2>(_val), _1)]
				                             >> *burn_param_card_layout
				                             >> *burn_dependency
				                             >> '}';
											 
		//BOOST_SPIRIT_DEBUG_NODE(node);
		//BOOST_SPIRIT_DEBUG_NODE(param_map);
		//BOOST_SPIRIT_DEBUG_NODE(param_array);
		//BOOST_SPIRIT_DEBUG_NODE(param_generic);
		//BOOST_SPIRIT_DEBUG_NODE(quoted_string);

		//BOOST_SPIRIT_DEBUG_NODE(array_value);
		//BOOST_SPIRIT_DEBUG_NODE(burn_properties);
		//BOOST_SPIRIT_DEBUG_NODE(burn_dependency);
		//BOOST_SPIRIT_DEBUG_NODE(burn_param_card_layout);



	}

	qi::rule<Iterator, XNodeParamMap(), ascii::space_type> xprot;
	qi::rule<Iterator, XNodeParamMap(), ascii::space_type> param_map;
	qi::rule<Iterator, XNodeParamArray(), ascii::space_type> param_array;
	qi::rule<Iterator, XNode(), ascii::space_type> node;
	qi::rule<Iterator, XNodeParamValue(), ascii::space_type> param_generic;
	qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
	qi::rule<Iterator, XNodeArrayValue(), ascii::space_type> array_value;
	qi::rule<Iterator, void(), ascii::space_type> burn_properties;
	qi::rule<Iterator, void(), ascii::space_type> burn_param_card_layout;
	qi::rule<Iterator, void(), ascii::space_type> burn_dependency;
	qi::rule<Iterator, void(), ascii::space_type> burn_protocol_composer;
	qi::real_parser<double, qi::strict_real_policies<double> > strict_double;


};

int ParseXProtocol(const std::string& input, XNode& output)
{
	typedef XProtocol::XNodeGrammar<std::string::const_iterator> XNodeGrammar;
	XNodeGrammar xprot; // Our grammar
	//XProtocol::XNode ast; // Our tree
	using boost::spirit::ascii::space;
	std::string::const_iterator iter = input.begin();
	std::string::const_iterator end = input.end();
	bool r = phrase_parse(iter, end, xprot, space, boost::get<XProtocol::XNodeParamMap>(output));

	if (!r || (iter != end))
	{
//	    std::cout << input << std::endl;
		return -1;
	}

	XProtocol::XNodeParamMap t =  boost::get<XProtocol::XNodeParamMap>(boost::get<XProtocol::XNodeParamMap>(output).children_[0]);
	output = t;
	return 0;
}

}
