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
# a floating point number or integer.  replace string by float
number = pp.Regex('[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?').setParseAction(lambda toks: [float(toks[0])])
# property values can either be a string or an int/double.
prop_value = quoted_string | number
# property dictionary is a list of name/value pairs
prop_dict = pp.dictOf(quoted_string, prop_value)
# header section is HEADER followed by properties.
header_section = pp.Literal("HEADER").suppress() + prop_dict

# type section definitions
# type length is either * for variable, or DOUBLE/SINGLE.
type_length = pp.Literal('*') | pp.Literal('DOUBLE') | pp.Literal('SINGLE')
# type is either string or float
type_name = pp.Literal('STRING') | pp.Literal('FLOAT')
# an array type is defined by presence of the string "ARRAY ( * )"  just return a single True token.
is_array = (pp.Literal('ARRAY') + pp.Literal('(') + pp.Literal('*') +
            pp.Literal(')')).setParseAction(lambda toks: [True])
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
            # NOTE: we group array so they don't get merged with others.
            self.parser = (pp.Literal('(').suppress() +
                           pp.Group(pp.ZeroOrMore(self.parser)) +
                           pp.Literal(')').suppress())

    def __repr__(self):
        arr_str = '[]' if self.is_array else ''
        return '%s(%s)%s' % (self.type_name, self.type_len, arr_str)


class StructDef(object):
    """A class that prepresents a struct type."""

    def __init__(self, parse_result):
        self.members = [ClassDef(v) for v in parse_result['members']]

        # struct value parse is just the members' value parsers in order surrounding by parenthesis
        value_parser = self.members[0].class_type.parser
        for cdef in self.members[1:]:
            value_parser += cdef.class_type.parser
        self.parser = (pp.Literal('(').suppress() +
                       value_parser +
                       pp.Literal(')').suppress())

        # modify return value to be a key-value pair list
        def modify_tokens(toks):
            ans = {}
            for idx, m in enumerate(self.members):
                val = toks[idx]
                if isinstance(val, pp.ParseResults):
                    # array value, convert to list
                    val = val.asList()
                ans[m.class_name] = val
            return ans

        self.parser = self.parser.setParseAction(modify_tokens)

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
    # NOTE: use setParseAction to keep dictionary structure intact.
    prop_parser = (pp.Literal('PROP').suppress() +
                   pp.Literal('(').suppress() +
                   prop_dict +
                   pp.Literal(')')).setParseAction(lambda toks: toks.asDict())

    # NOTE: Group value and property separately so properties field won't merge
    # with value field.
    value_parser = pp.Group(value_parser)('value') + pp.Group(pp.Optional(prop_parser, default={}))('properties')

    # unwrap the group so asDict() converts correctly.
    def format_value(toks):
        return {'value': toks[0][0], 'properties': toks[1][0]}

    value_parser.setParseAction(format_value)

    # VALUE section is just a list of variable name followed by its value/properties.
    val_dict = pp.dictOf(quoted_string, value_parser)

    # add VALUE and END tags
    val_section = pp.Literal('VALUE').suppress() + val_dict + pp.Literal('END').suppress()
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
    return results.asDict(), properties.asDict()
