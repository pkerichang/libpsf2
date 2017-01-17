# -*- coding: utf-8 -*-

"""This module defines pyparsing grammars used to parse ASCII PSF files."""

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
# noinspection PyCompatibility
from builtins import *

import pyparsing as pp

# header section definitions
# a quoted string.
quoted_string = pp.QuotedString('"', escChar='\\', unquoteResults=True)
# a floating point number or integer
number = pp.Regex('[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?').setParseAction(lambda toks: float(toks[0]))
# property values can either be a string or an int/double.
prop_value = quoted_string | number
# property entry is name followed by value
prop_entry = (quoted_string + prop_value).setParseAction(lambda toks: (toks[0], toks[1]))
# property dictionary is a list of name/value pairs
prop_dict = pp.ZeroOrMore(prop_entry).setParseAction(lambda toks: dict(toks.asList()))
# header section is HEADER followed by properties.
header_section = pp.Literal("HEADER").suppress() + prop_dict
# set header to directly return property dictionary.
header_section = header_section.setParseAction(lambda toks: toks[0])

# type section definitions
# type length is either * for variable, or DOUBLE/SINGLE.
type_length = pp.Literal('*') | pp.Literal('DOUBLE') | pp.Literal('SINGLE')
# type is either string or float
type_name = pp.Literal('STRING') | pp.Literal('FLOAT')
# an array type is defined by presence of the string "ARRAY ( * )"
is_array = (pp.Literal('ARRAY').setParseAction(lambda toks: True) +
            pp.Literal('(').suppress() +
            pp.Literal('*').suppress() +
            pp.Literal(')').suppress())
# a type definition is an optional array identifier, followed by type name and length.
type_info = (pp.Optional(is_array, default=False)('is_array') +
             type_name('type_name') +
             type_length('type_len'))
# a simple class definition is a class name followed by simple type.
class_def_simple = (pp.Group(quoted_string)('class_name') +
                    pp.Group(type_info)('class_type'))
# a struct is a list of inner simple class definitions, enclosed in STRUCT( ), followed
# by optional PROP( ) section.
struct_def = (pp.Literal('STRUCT').suppress() +
              pp.Literal('(').suppress() +
              pp.Group(pp.OneOrMore(pp.Group(class_def_simple)))('members') +
              pp.Literal(')').suppress() +
              pp.Optional(pp.Literal('PROP') + pp.Literal('(') +
                          pp.Literal('"key"') + quoted_string +
                          pp.Literal(')')).suppress())
# a class definition is a class name followed by a struct or simple type.
class_def = (pp.Group(quoted_string)('class_name') +
             pp.Group(type_info | struct_def)('class_type'))
# type section is TYPE followed by a list of class definitions.
type_section = pp.Literal('TYPE').suppress() + pp.OneOrMore(pp.Group(class_def))

# meaningful ASCII PSF files always start with header and type section.
preamble = header_section('header') + type_section('type')


class TypeInfo(object):
    """A class that represents a data type."""

    def __init__(self, parse_result):
        self.type_name = parse_result['type_name']
        self.type_len = parse_result['type_len']
        self.is_array = parse_result['is_array'][0]

        # create parser
        if self.type_name == 'STRING':
            # string type
            self.parser = quoted_string
        elif self.type_name == 'FLOAT':
            # float type
            self.parser = number
        else:
            raise ValueError('Unsupported primitive type %s' % self.type_name)

        if self.is_array:
            # wrap parser in array
            self.parser = (pp.Literal('(').suppress() +
                           pp.ZeroOrMore(pp.Group(self.parser)) +
                           pp.Literal(')').suppress())

    def __repr__(self):
        arr_str = '[]' if self.is_array else ''
        return '%s(%s)%s' % (self.type_name, self.type_len, arr_str)


class StructDef(object):
    """A class that prepresents a struct type."""

    def __init__(self, parse_result):
        self.members = [ClassDef(v) for v in parse_result['members']]

        # struct value parse is just the members' value parsers in order...
        print(self.members)
        value_parser = self.members[0].class_type.parser(self.members[0].class_name)
        for cdef in self.members[1:]:
            value_parser += cdef.class_type.parser(cdef.class_name)
        # ...surround by parennthesis
        self.parser = (pp.Literal('(').suppress() +
                       value_parser +
                       pp.Literal(')').suppress())

        # set parser to return dictionary.
        # NOTE: for some reason, if pyparsing.QuotedString strips quotes and parses empty string,
        # it will not show up in parsed results.  This lambda function fixes that.
        self.parser = self.parser.setParseAction(lambda toks: {m.class_name: toks.get(m.class_name, '')
                                                               for m in self.members})

    def __repr__(self):
        member_str = '\n\t'.join((repr(m) for m in self.members))
        return 'STRUCT(\n\t' + member_str + ')'


class ClassDef(object):
    """A class that prepresents a class definition."""

    def __init__(self, parse_result):
        self.class_name = parse_result['class_name'][0]

        # check if see if this class is a simple type or a struct type.
        my_type = parse_result['class_type']
        if 'members' in my_type:
            self.class_type = StructDef(my_type)
        else:
            self.class_type = TypeInfo(my_type)

        # class value parser is just class name followed by class type parser.
        self.parser = (pp.Literal('"%s"' % self.class_name).suppress() +
                       self.class_type.parser)

    def __repr__(self):
        return "%s: %s" % (self.class_name, repr(self.class_type))


def make_value_section_parser(type_results):
    """"Create a parser that parses the value section.

    Because the VALUE section format depends on the types defined in the TYPE section,
    this method will construct a pyparsing parser that parses the VALUE section based
    on TYPE section parsed results.

    Parameters
    ----------
    type_results : pyparsing.ParseResults
        the parsed results from the type section.
    """

    # construct ClassDef objects from parsed TYPE section.
    class_def_list = [ClassDef(v) for v in type_results]

    # In VALUE section, the value has to be one of the class definition defined
    # in the TYPE section, so we OR all the value parsers together.
    value_parser = class_def_list[0].parser
    for cdef in class_def_list[1:]:
        value_parser |= cdef.parser

    # There can be an optional PROP section at the end.
    prop_parser = (pp.Literal('PROP').suppress() +
                   pp.Literal('(').suppress() +
                   prop_dict +
                   pp.Literal(')'))
    value_parser = value_parser + pp.Optional(prop_parser)('properties')

    # VALUE section is just a list of variable name followed by its value/properties.
    value_entry = (quoted_string + value_parser).setParseAction(lambda toks: (toks[0], toks[1]))
    # convert parsed output to dictionary.
    val_dict = pp.OneOrMore(value_entry).setParseAction(lambda toks: dict(toks.asList()))

    # add VALUE and END tags
    val_section = pp.Literal('VALUE').suppress() + val_dict + pp.Literal('END').suppress()
    val_section = val_section.setParseAction(lambda toks: toks[0])
    return val_section


def parse_psf_ascii(file_name):
    """Parse the given ASCII PSF file and return the content."""

    with open(file_name, 'r') as f:
        content = f.read()

    # parse the header and type section first.
    parse_results, _, pos = list(preamble.scanString(content))[0]
    properties = parse_results['header']
    # construct value section parser.
    type_results = parse_results['type']
    val_parser = make_value_section_parser(type_results)

    # parser rest of the file
    results = val_parser.parseString(content[pos:], parseAll=True)

    # return values and header properties
    return results[0], properties
