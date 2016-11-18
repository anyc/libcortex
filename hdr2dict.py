#! /usr/bin/env python

import sys
import clang.cindex

def indent(level):
    return '  '*level

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
		return "I"
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

	#st = " "
	#if cursor.type:
		#if cursor.type.kind == clang.cindex.TypeKind.POINTER:
			#st += "myptr "
		
		#st += cursor.type.spelling+" "
		#st += cursor.type.get_canonical().spelling+" "
		#st += str(cursor.type.get_size())+" "
	
	if output and cursor.type.kind == clang.cindex.TypeKind.RECORD:
		if spelling not in addition:
			#addition.append(spelling)
			pass
	
	if output and cursor.kind == clang.cindex.CursorKind.FIELD_DECL:
		print ""
		#print indent(level) + spelling, '<' + str(kind) + '>'
		#print indent(level+1) + '"'  + displayname + '"' + st
		
		#is_struct = False
		##print(cursor.type.get_canonical().kind)
		##if (cursor.type.get_canonical().kind == clang.cindex.TypeKind.POINTER or
		#if cursor.type.get_canonical().kind == clang.cindex.TypeKind.RECORD:
			#is_struct = True
			##print(cursor.canonical.type.spelling)
			##for c in cursor.get_children():
				##print(c.kind)
				##if c.kind == clang.cindex.CursorKind.TYPE_REF:
					##print(c.referenced.type.kind)
					###if c.referenced.type.kind == clang.cindex.TypeKind.RECORD:
						###print(c.referenced.spelling)
					##if c.referenced.kind == clang.cindex.CursorKind.STRUCT_DECL:
						##is_struct = True
					
		
		typ = cursor.type.get_canonical()
		#print(typ.get_declaration())
		#print(cursor.type.kind)
		
		print(indent(level) + di + " = crtx_alloc_item(dict);")
		
		if typ.kind == clang.cindex.TypeKind.POINTER:
			if typ.spelling.find("char *") > -1:
				print(indent(level) + "crtx_fill_data_item("+di+", 's', \""+spelling+"\", "+prefix+spelling+", strlen("+prefix+spelling+"), DIF_DATA_UNALLOCATED);")
			else:
				#print(cursor.type.get_pointee().get_declaration().type.kind)
				#print(cursor.type.get_pointee().kind)
				#for c in cursor.get_children():
					#print(c.type.kind.spelling)
				#if typ.get_pointee().spelling == "void":
				if cursor.type.get_pointee().get_declaration().type.kind == clang.cindex.TypeKind.RECORD:
					print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
					
					prefix = prefix + spelling + "->"
					
					output_cursor_and_children(cursor.type.get_pointee().get_declaration(), level+1, di, prefix)
				else:
					print(indent(level) + "crtx_fill_data_item("+di+", 'p', \""+spelling+"\", "+prefix+spelling+", 0, 0);")
		#elif typ.kind == clang.cindex.TypeKind.ENUM:
			##print(typ.get_canonical().kind)
			##print(typ.get_size())
			#print(indent(level) + "crtx_fill_data_item(di, 'I', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
		elif typ.kind == clang.cindex.TypeKind.CONSTANTARRAY:
			print(indent(level) + "crtx_fill_data_item("+di+", 'D', \""+spelling+"\", 0, 0, 0);")
			print("")
			#print(typ.get_array_size())
			#print(typ.get_array_element_type().get_canonical().kind)
			
			#if typ.get_array_element_type().get_canonical().kind == clang.cindex.TypeKind.ENUM:
				#subtype = "I"
			#else:
			subtype = type_clang2crtx(typ.get_array_element_type().get_canonical())
			
			print(indent(level) + di+"->ds = crtx_init_dict(\""+(subtype*typ.get_array_size())+"\", "+str(typ.get_array_size())+", 0);")
			print(indent(level) + "{")
			#print(indent(level+1) + "struct crtx_dict *di2 = di->ds;")
			for i in range(typ.get_array_size()):
				#print(indent(level+1) + "di = crtx_alloc_item(di2);")
				
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
				
				output_cursor_and_children(cursor.type.get_canonical().get_declaration(), level+1, di, prefix)
			else:
				
				#print(cursor.type.get_canonical().spelling)
				print(typ.kind)
				
				for c in cursor.get_children():
					if c.kind == clang.cindex.CursorKind.TYPE_REF:
						#print(c.referenced.type.kind)
						#if c.referenced.type.kind == clang.cindex.TypeKind.RECORD:
							#print(c.referenced.spelling)
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
	
	#if cursor.kind.is_reference() and cursor.type.kind == clang.cindex.TypeKind.RECORD:
		#if output:
			#print indent(level) + 'reference to:'
		##if cursor.referenced.kind == clang.cindex.CursorKind.STRUCT_DECL: # and cursor.type.kind != clang.cindex.TypeKind.POINTER:
			##output_cursor_and_children(cursor.referenced, level+1)
		##else:
		#output_cursor(cursor.referenced, level+1, di, prefix)

def output_cursor_and_children(cursor, level=0, di=None, prefix=""):
	global output, struct
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct and cursor.type.kind == clang.cindex.TypeKind.RECORD:
			output = True
			print("struct crtx_dict *crtx_"+struct+"2dict(struct "+struct+" *ptr)")
			prefix = "ptr->"
	
	
	output_cursor(cursor, level, di, prefix)
	
	if cursor.type.kind == clang.cindex.TypeKind.RECORD and cursor.kind == clang.cindex.CursorKind.UNION_DECL:
		# TODO
		pass
	
	if (
		(cursor.type.kind == clang.cindex.TypeKind.RECORD and cursor.kind == clang.cindex.CursorKind.STRUCT_DECL)
			or
		cursor.kind == clang.cindex.CursorKind.TRANSLATION_UNIT):
		
		has_children = False;
		for c in cursor.get_children():
			if (not has_children):
				if output:
					#print indent(level) + '{' + "\n"+ indent(level+1) + "struct crtx_dict *dict = di->ds; struct crtx_dict_item *di;\n"
					print indent(level) + '{'
					if di is None:
						print indent(level+1) + "struct crtx_dict *dict = crtx_init_dict(0, 0, 0);"
						print indent(level+1) + "struct crtx_dict_item *di;\n"
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
			print indent(level) + '}'
		
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct:
			output = False
			print("\n\n")

index = clang.cindex.Index.create()
tu = index.parse(sys.argv[1], options=1)

addition = [sys.argv[2]]
output = False

print("#include <string.h>")
print("#include \""+sys.argv[1]+"\"\n")
print("#include \"dict.h\"\n")

i=0
while i < len(addition):
	struct = addition[i]
	
	output_cursor_and_children(tu.cursor, 0, None)
	
	i += 1
