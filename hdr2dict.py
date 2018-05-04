#! /usr/bin/env python

#
# Mario Kicherer (dev@kicherer.org) 2016
#

import sys, argparse, os
import clang.cindex

def indent(level):
    return '\t'*level

def type_clang2crtx(typ):
	if (typ.spelling.find("int") > -1 or 
		typ.spelling.find("long") > -1 or
		typ.spelling.find("char") > -1 or
		typ.spelling.find("short") > -1
		):
		if typ.get_size() <= 4:
			if typ.spelling.find("unsigned") > -1:
				return "u"
			else:
				return "i"
		if typ.get_size() == 8:
			if typ.spelling.find("unsigned") > -1:
				return "U"
			else:
				return "I"
	elif typ.kind == clang.cindex.TypeKind.ENUM:
		return "i"
	elif typ.spelling.find("double") > -1 or typ.spelling.find("float") > -1:
		return "d"
	elif typ.kind == clang.cindex.TypeKind.RECORD:
		return "D"
	else:
		None

def output_cursor(cursor, level, di, prefix=""):
	global output, struct
	
	spelling = ''
	displayname = ''

	if cursor.spelling:
		spelling = cursor.spelling
	if cursor.displayname:
		displayname = cursor.displayname
	kind = cursor.kind;
	
	if output and cursor.type.kind == clang.cindex.TypeKind.RECORD:
		if spelling not in args.structs:
			#addition.append(spelling)
			pass
	
	if output and cursor.kind == clang.cindex.CursorKind.FIELD_DECL:
		print ""
		
		typ = cursor.type.get_canonical()
		
		if typ.kind == clang.cindex.TypeKind.POINTER:
			if typ.spelling.find("char *") > -1:
				print(indent(level) + "if ("+prefix+spelling+") {")
				level += 1
				
				#print(indent(level) + di + " = crtx_alloc_item(dict);")
				
				print(indent(level) + "crtx_dict_new_item(dict, 's', \""+spelling+"\", "+prefix+spelling+", strlen("+prefix+spelling+"), CRTX_DIF_DONT_FREE_DATA);")
				#print(indent(level) + "crtx_fill_data_item("+di+", 's', \""+spelling+"\", "+prefix+spelling+", 0, DIF_COPY_STRING);")
				
				level -= 1
				print(indent(level) + "}")
			else:
				print(indent(level) + di + " = crtx_alloc_item(dict);")
				
				#print(cursor.spelling, cursor.type.get_pointee().get_canonical().get_declaration().type.kind)
				has_children = False
				for c in cursor.type.get_pointee().get_canonical().get_declaration().get_children():
					has_children = True
					break
				
				if has_children and cursor.type.get_pointee().get_canonical().get_declaration().type.kind == clang.cindex.TypeKind.RECORD:
					print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
					
					if level < args.max_recursion:
						output_cursor_and_children(cursor.type.get_pointee().get_canonical().get_declaration(), level, di, prefix + spelling, "->")
					else:
						structname = cursor.type.get_pointee().get_canonical().spelling.replace("struct ", "")
						if structname in args.structs:
							print(indent(level) + "crtx_"+str(structname)+"2dict("+prefix + spelling+", "+di+"->dict);")
						else:
							print(indent(level) + "// crtx_"+str(structname)+"2dict("+prefix + spelling+", "+di+"->dict);")
						print("")
				else:
					print(indent(level) + "crtx_fill_data_item("+di+", 'p', \""+spelling+"\", "+prefix+spelling+", 0, CRTX_DIF_DONT_FREE_DATA);")
			
		elif typ.kind == clang.cindex.TypeKind.CONSTANTARRAY:
			print(indent(level) + di + " = crtx_alloc_item(dict);")
			
			subtype = type_clang2crtx(typ.get_array_element_type().get_canonical())
			
			if typ.get_array_element_type().get_canonical().kind != clang.cindex.TypeKind.CHAR_S:
				print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
				print("")
				
				print(indent(level) + "// "+str(typ.get_array_element_type().get_canonical().kind))
				print(indent(level) + di+"->dict = crtx_init_dict(\""+(subtype*typ.get_array_size())+"\", "+str(typ.get_array_size())+", 0);")
				print(indent(level) + "{")
				
				for i in range(typ.get_array_size()):
					print(indent(level+1) + "crtx_fill_data_item(&"+di+"->dict->items["+str(i)+"], '"+subtype+"', 0, "+prefix+spelling+"["+str(i)+"], sizeof("+prefix+spelling+"["+str(i)+"]), 0);")
				print(indent(level) + "}")
			else:
				print(indent(level) + "crtx_fill_data_item("+di+", 's', \""+spelling+"\", "+prefix+spelling+", sizeof("+prefix+spelling+"), CRTX_DIF_CREATE_DATA_COPY);")
				#print("")
		elif typ.kind == clang.cindex.TypeKind.ENUM:
			if cursor.type.get_declaration().kind == clang.cindex.CursorKind.TYPEDEF_DECL:
				cur = list(cursor.type.get_declaration().get_children())[0]
			else:
				cur = cursor
			#print(cursor.type.get_declaration().kind)
			#for c in cursor.type.get_declaration().get_children():
				#print(c, c.spelling, c.displayname)
				#print(c.type, c.kind)
				#print(c.enum_type.kind, clang.cindex.TypeKind.UINT)
				#for cc in c.get_children():
					#print(cc.spelling, cc.displayname, cc.enum_value)
			
			
			crtx_typ = type_clang2crtx(cur.type.get_canonical())
			if crtx_typ in ["u", "U", "i", "I", "d"]:
				#print(indent(level) + di + " = crtx_alloc_item(dict);")
				print(indent(level) + "crtx_dict_new_item(dict, '"+crtx_typ+"', \""+spelling+"\", "+prefix+spelling+", sizeof("+prefix+spelling+"), 0);")
			
			print("")
			#print(indent(level) + di + " = crtx_alloc_item(dict);")
			print(indent(level) + "switch ("+prefix+spelling+") {")
			for enum in cur.get_children():
				#print(enum.spelling, enum.displayname, enum.enum_value)
				print(indent(level+1) + "case "+enum.displayname+":")
				print(indent(level+2) + "crtx_dict_new_item(dict, 's', \""+spelling+"\", \""+enum.displayname+"\", sizeof(\""+enum.displayname+"\"), CRTX_DIF_DONT_FREE_DATA);")
				print(indent(level+2) + "break;")
			print(indent(level) + "}")
		else:
			crtx_typ = type_clang2crtx(typ)
			
			if crtx_typ in ["u", "U", "i", "I", "d"]:
				print(indent(level) + "crtx_dict_new_item(dict, '"+crtx_typ+"', \""+spelling+"\", "+prefix+spelling+", sizeof("+prefix+spelling+"), 0);")
			
			elif crtx_typ == "D":
				print(indent(level) + di + " = crtx_alloc_item(dict);")
				print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
				
				if level < args.max_recursion:
					print("")
					output_cursor_and_children(cursor.type.get_canonical().get_declaration(), level, di, prefix + spelling, ".")
				else:
					structname = cursor.type.get_canonical().spelling.replace("struct ", "")
					if structname in args.structs:
						print(indent(level) + "crtx_"+str(structname)+"2dict(&"+prefix + spelling+", "+di+"->dict);")
					else:
						print(indent(level) + "// crtx_"+str(structname)+"2dict(&"+prefix + spelling+", "+di+"->dict);")
					print("")
			else:
				print(typ.kind)
				
				for c in cursor.get_children():
					if c.kind == clang.cindex.CursorKind.TYPE_REF:
						print(c.referenced.kind)
						if c.referenced.kind == clang.cindex.CursorKind.STRUCT_DECL:
							print(c.kind, c.referenced.kind)
				
				st = " "
				if cursor.type:
					if cursor.type.kind == clang.cindex.TypeKind.POINTER:
						st += "myptr "
					
					st += cursor.type.spelling+" "
					st += cursor.type.get_canonical().spelling+" "
					st += str(cursor.type.get_size())+" "
				print indent(level) + spelling, '<' + str(kind) + '>'
				print indent(level+1) + '"'  + displayname + '"' + st

def output_cursor_and_children(cursor, level=0, di=None, prefix="", sep=""):
	global output, struct
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct and cursor.type.kind == clang.cindex.TypeKind.RECORD:
			output = True
			print("char crtx_"+struct+"2dict(struct "+struct+" *ptr, struct crtx_dict **dict_ptr)")
			prefix = "ptr"
			sep = "->"
	
	
	output_cursor(cursor, level, di, prefix+sep)
	
	#if cursor.type.kind == clang.cindex.TypeKind.RECORD and cursor.kind == clang.cindex.CursorKind.UNION_DECL:
		#print("/* TODO union */")
	
	if (
		(cursor.type.kind == clang.cindex.TypeKind.RECORD) # and cursor.kind == clang.cindex.CursorKind.STRUCT_DECL)
			or
		cursor.kind == clang.cindex.CursorKind.TRANSLATION_UNIT):
		
		if cursor.kind == clang.cindex.CursorKind.UNION_DECL and output:
			print("")
			print(indent(level) + "/* TODO union: ")
		
		has_children = False;
		for c in cursor.get_children():
			if (not has_children):
				if output:
					if prefix != "" and sep == "->" and not (cursor.spelling == struct):
						print indent(level) + 'if ('+prefix+') {'
					else:
						print indent(level) + '{'
					if di is None:
						#print indent(level+1) + "struct crtx_dict *dict = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict *dict;"
						print indent(level+1) + "struct crtx_dict_item *di;\n"
						#print(indent(level+1) + "if (*dict_ptr) {")
						#print(indent(level+2) + "dict = *dict_ptr;")
						#print(indent(level+1) + "} else {")
						print(indent(level+1) + "if (! *dict_ptr)")
						print(indent(level+2) + "*dict_ptr = crtx_init_dict(0, 0, 0);")
						print(indent(level+1) + "dict = *dict_ptr;")
						#print(indent(level+1) + "}")
						di = "di";
					elif di == "di":
						print indent(level+1) + di + "->dict = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict *dict = "+di+"->dict;"
						di = "di2";
						print indent(level+1) + "struct crtx_dict_item *"+di+";"
					elif di == "di2":
						print indent(level+1) + di + "->dict = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict *dict = "+di+"->dict;"
						di = "di";
						print indent(level+1) + "struct crtx_dict_item *"+di+";"
						
				has_children = True
			#if level < args.max_recursion:
			output_cursor_and_children(c, level+1, di, prefix, sep)
		
		if has_children and output:
			if cursor.spelling == struct:
				print indent(level+1) + ''
				print indent(level+1) + 'return 0;'
			print indent(level) + '}'
		
		if cursor.kind == clang.cindex.CursorKind.UNION_DECL and output:
			print(indent(level) + "*/")
		
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct:
			output = False
			print("\n\n")

parser = argparse.ArgumentParser(description='Code generator that converts C structs into cortex dicts')
parser.add_argument('--max-recursion', type=int, default=1, help='')
parser.add_argument('--structs', action='append', help='')
parser.add_argument('hdr_file', nargs=1, help='')

args = parser.parse_args()

if not args.structs:
	parser.print_help()
	sys.exit(1)

index = clang.cindex.Index.create()
tu = index.parse(args.hdr_file[0], options=1)

#addition = sys.argv[2:]
output = False

print("/*")
print(" * This file has been generated using:")# "+sys.argv[1]+" */")
print(" *  "+" ".join(sys.argv)+"")
print(" */")
print("")
print("#include <string.h>")
print("// #include <"+os.path.basename(args.hdr_file[0])+">")
print("")
print("#include \"dict.h\"\n")

i=0
while i < len(args.structs):
	struct = args.structs[i]
	
	output_cursor_and_children(tu.cursor, -1)
	
	i += 1
