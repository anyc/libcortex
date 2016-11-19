#! /usr/bin/env python

import sys
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
		if spelling not in addition:
			#addition.append(spelling)
			pass
	
	if output and cursor.kind == clang.cindex.CursorKind.FIELD_DECL:
		print ""
		
		typ = cursor.type.get_canonical()
		
		print(indent(level) + di + " = crtx_alloc_item(dict);")
		
		if typ.kind == clang.cindex.TypeKind.POINTER:
			if typ.spelling.find("char *") > -1:
				print(indent(level) + "crtx_fill_data_item("+di+", 's', \""+spelling+"\", "+prefix+spelling+", strlen("+prefix+spelling+"), DIF_DATA_UNALLOCATED);")
				#print(indent(level) + "crtx_fill_data_item("+di+", 's', \""+spelling+"\", "+prefix+spelling+", 0, DIF_COPY_STRING);")
			else:
				if cursor.type.get_pointee().get_declaration().type.kind == clang.cindex.TypeKind.RECORD:
					print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
					
					prefix = prefix + spelling + "->"
					
					output_cursor_and_children(cursor.type.get_pointee().get_declaration(), level, di, prefix)
				else:
					print(indent(level) + "crtx_fill_data_item("+di+", 'p', \""+spelling+"\", "+prefix+spelling+", 0, DIF_DATA_UNALLOCATED);")
		elif typ.kind == clang.cindex.TypeKind.CONSTANTARRAY:
			print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
			print("")
			
			subtype = type_clang2crtx(typ.get_array_element_type().get_canonical())
			
			print(indent(level) + di+"->ds = crtx_init_dict(\""+(subtype*typ.get_array_size())+"\", "+str(typ.get_array_size())+", 0);")
			print(indent(level) + "{")
			
			for i in range(typ.get_array_size()):
				print(indent(level+1) + "crtx_fill_data_item(&"+di+"->ds->items["+str(i)+"], '"+subtype+"', 0, "+prefix+spelling+"["+str(i)+"], sizeof("+prefix+spelling+"["+str(i)+"]), 0);")
			print(indent(level) + "}")
		else:
			crtx_typ = type_clang2crtx(typ)
			
			if crtx_typ in ["u", "U", "i", "I", "d"]:
				print(indent(level) + "crtx_fill_data_item("+di+", '"+crtx_typ+"', \""+spelling+"\", "+prefix+spelling+", sizeof("+prefix+spelling+"), 0);")
			
			elif crtx_typ == "D":
				print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
				print("")
				
				prefix = prefix + spelling + "."
				
				output_cursor_and_children(cursor.type.get_canonical().get_declaration(), level, di, prefix)
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

def output_cursor_and_children(cursor, level=0, di=None, prefix=""):
	global output, struct
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct and cursor.type.kind == clang.cindex.TypeKind.RECORD:
			output = True
			print("char crtx_"+struct+"2dict(struct "+struct+" *ptr, struct crtx_dict **dict_ptr)")
			prefix = "ptr->"
	
	
	output_cursor(cursor, level, di, prefix)
	
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
						print indent(level+1) + di + "->ds = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict *dict = "+di+"->ds;"
						di = "di2";
						print indent(level+1) + "struct crtx_dict_item *"+di+";"
					elif di == "di2":
						print indent(level+1) + di + "->ds = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict *dict = "+di+"->ds;"
						di = "di";
						print indent(level+1) + "struct crtx_dict_item *"+di+";"
						
				has_children = True
			output_cursor_and_children(c, level+1, di, prefix)
		
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

index = clang.cindex.Index.create()
tu = index.parse(sys.argv[1], options=1)

addition = sys.argv[2:]
output = False

print("/* This file has been generated with hdr2dict.py */")
print("#include <string.h>")
print("// #include \""+sys.argv[1]+"\"\n")
print("#include \"dict.h\"\n")

i=0
while i < len(addition):
	struct = addition[i]
	
	output_cursor_and_children(tu.cursor, -1, None)
	
	i += 1
