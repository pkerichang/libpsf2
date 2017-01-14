#!/usr/bin/env python

import pyparsing as pp

unquoted_string = pp.Word(pp.printables)
quoted_string = pp.QuotedString('"', escChar='\\')
general_string = quoted_string | unquoted_string

prop_dict = pp.dictOf(quoted_string, general_string)
header_section = pp.Literal("HEADER").suppress() + prop_dict

type_length = pp.Literal('*') | pp.Literal('DOUBLE') | pp.Literal('SINGLE')
type_name = pp.Literal('STRING') | pp.Literal('FLOAT')
is_array = (pp.Literal('ARRAY').setParseAction(lambda toks: True) +
            pp.Literal('(').suppress() +
            pp.Literal('*').suppress() +
            pp.Literal(')').suppress())

type_info = (pp.Optional(is_array, default=False)('is_array') +
             type_name('type_name') +
             type_length('type_len'))

class_def_simple = (pp.Group(quoted_string)('class_name') +
                    pp.Group(type_info)('class_type'))

struct_def = (pp.Literal('STRUCT').suppress() + 
              pp.Literal('(').suppress() +
              pp.Group(pp.OneOrMore(pp.Group(class_def_simple)))('members') +
              pp.Literal(')').suppress())

class_def = (pp.Group(quoted_string)('class_name') +
            pp.Group(type_info | struct_def)('class_type'))

type_section = pp.Literal('TYPE').suppress() + pp.OneOrMore(pp.Group(class_def))

preamble = pp.Group(header_section)('header') + pp.Group(type_section)('type')


class TypeInfo(object):
    def __init__(self, parse_result):
        self.type_name = parse_result['type_name']
        self.type_len = parse_result['type_len']
        self.is_array = parse_result['is_array'][0]

        # create parser
        if self.type_name == 'STRING':
            # string type
            self.parser = quoted_string
        elif self.type_name == 'FLOAT':
            # regular expression for a float
            self.parser = pp.Regex('[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?')
            # make parser return float.
            self.parser = self.parser.setParseAction(lambda toks: float(toks[0]))
        else:
            raise ValueError('Unsupported primitive type %s' % self.name)

        if self.is_array:
            # wrap parser in array
            self.parser = (pp.Literal('(').suppress() +
                           pp.ZeroOrMore(pp.Group(self.parser)) +
                           pp.Literal(')').suppress())


class ClassDefSimple(object):
    def __init__(self, parse_result):
        self.class_name = parse_result['class_name'][0]
        self.class_type = TypeInfo(parse_result['class_type'])


class StructDef(object):
    def __init__(self, parse_result):
        self.members = [ClassDefSimple(v) for v in parse_result['members']]
        value_parser = self.members[0].class_type.parser(self.members[0].class_name)
        for class_def in self.members[1:]:
            value_parser += class_def.class_type.parser(class_def.class_name)
        
        self.parser = (pp.Literal('(').suppress() +
                       value_parser +
                       pp.Literal(')').suppress())

class ClassDef(object):
    def __init__(self, parse_result):
        self.class_name = parse_result['class_name'][0]
        my_type = parse_result['class_type']
        if 'members' in my_type:
            self.class_type = StructDef(my_type)
        else:
            self.class_type = TypeInfo(my_type)

        self.parser = (pp.Literal('"%s"' % self.class_name).suppress() +
                       self.class_type.parser)

        
with open('../logFile', 'r') as f:
    content = f.read()

parse_results, _, pos = list(preamble.scanString(content))[0]
type_results = parse_results['type']

class_def_list = [ClassDef(v) for v in type_results]

value_parser = class_def_list[0].parser
for class_def in class_def_list[1:]:
    value_parser |= class_def.parser

prop_parser = (pp.Literal('PROP').suppress() +
               pp.Literal('(').suppress() +
               prop_dict +
               pp.Literal(')'))

value_parser = value_parser('value') + pp.Optional(prop_parser)('properties')
               
val_dict = pp.dictOf(quoted_string, value_parser)
value_section = pp.Literal('VALUE').suppress() + val_dict

results = value_section.parseString(content[pos:])
