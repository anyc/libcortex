#! /usr/bin/env python

import sys
import clang.cindex

def indent(level):
    return '  '*level

def output_cursor(cursor, level):
	global output, struct
	
	spelling = ''
	displayname = ''

	if cursor.spelling:
		spelling = cursor.spelling
	if cursor.displayname:
		displayname = cursor.displayname
	kind = cursor.kind;

	st = " "
	if cursor.type:
		if cursor.type.kind == clang.cindex.TypeKind.POINTER:
			st += "myptr "
		
		st += cursor.type.spelling+" "
		st += cursor.type.get_canonical().spelling+" "
		st += str(cursor.type.get_size())+" "
	
	if output and cursor.type.kind == clang.cindex.TypeKind.RECORD:
		if spelling not in addition:
			addition.append(spelling)
	
	if output and cursor.kind == clang.cindex.CursorKind.FIELD_DECL:
		print ""
		#print indent(level) + spelling, '<' + str(kind) + '>'
		#print indent(level+1) + '"'  + displayname + '"' + st
		
		type_spelling = cursor.type.get_canonical().spelling
		
		#if cursor.kind == clang.cindex.CursorKind.TYPE_REF:
			#ref = cursor
		#else:
			#ref = None
		#while ref:
		##if cursor.referenced:
			#print(cursor.kind, cursor.type.kind)
			#print(cursor.referenced.kind, cursor.referenced.type.kind)
			#if ref.kind == clang.cindex.CursorKind.TYPE_REF:
				#ref = ref.referenced
			#else:
				#ref = None
		
		is_struct = False
		if cursor.type.kind == clang.cindex.TypeKind.POINTER:
			for c in cursor.get_children():
				if c.kind == clang.cindex.CursorKind.TYPE_REF:
					#print(c.referenced.type.kind)
					#if c.referenced.type.kind == clang.cindex.TypeKind.RECORD:
						#print(c.referenced.spelling)
					if c.referenced.kind == clang.cindex.CursorKind.STRUCT_DECL:
						is_struct = True
					
		
		print(indent(level) + "di = crtx_alloc_item(dict);")
		if cursor.type.kind == clang.cindex.TypeKind.POINTER:
			if type_spelling == "char *":
				print(indent(level) + "crtx_fill_data_item(di, 's', \""+spelling+"\", ptr->"+spelling+", strlen(value), DIF_DATA_UNALLOCATED);")
			else:
				#if cursor.kind == clang.cindex.CursorKind.STRUCT_DECL:
			#if cursor.kind == clang.cindex.CursorKind.STRUCT_DECL:
				if is_struct:
					print(indent(level) + "crtx_fill_data_item(di, 'd', \""+spelling+"\", 0, 0, 0);")
				else:
					print(indent(level) + "crtx_fill_data_item(di, 'p', \""+spelling+"\", ptr->"+spelling+", 0, 0);")
		else:
			if (type_spelling.find("int") > -1 or 
				type_spelling.find("long") > -1 or
				type_spelling.find("char") > -1 or
				type_spelling.find("short") > -1
				):
				if cursor.type.get_size() <= 4:
					if type_spelling.find("unsigned") > -1:
						print(indent(level) + "crtx_fill_data_item(di, 'u', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
					else:
						print(indent(level) + "crtx_fill_data_item(di, 'i', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
				if cursor.type.get_size() == 8:
					if type_spelling.find("unsigned") > -1:
						print(indent(level) + "crtx_fill_data_item(di, 'U', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
					else:
						print(indent(level) + "crtx_fill_data_item(di, 'I', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
			elif type_spelling.find("double") > -1 or type_spelling.find("float") > -1:
				print(indent(level) + "crtx_fill_data_item(di, 'd', \""+spelling+"\", ptr->"+spelling+", sizeof(ptr->"+spelling+"), 0);")
	
	#print(cursor.kind.is_reference(), cursor.type.kind, clang.cindex.TypeKind.POINTER)
	if cursor.kind.is_reference() and cursor.type.kind == clang.cindex.TypeKind.RECORD:
		if output:
			print indent(level) + 'reference to:'
		#if cursor.referenced.kind == clang.cindex.CursorKind.STRUCT_DECL: # and cursor.type.kind != clang.cindex.TypeKind.POINTER:
			#output_cursor_and_children(cursor.referenced, level+1)
		#else:
		output_cursor(cursor.referenced, level+1)

def output_cursor_and_children(cursor, level=0):
	global output, struct
	
	if cursor.spelling and cursor.is_definition():
		if cursor.spelling == struct:
			output = True
			print("struct crtx_dict *crtx_"+struct+"2dict(struct "+struct+" *ptr)")
	
	
	output_cursor(cursor, level)

	if (cursor.type.kind == clang.cindex.TypeKind.RECORD or
		cursor.kind == clang.cindex.CursorKind.TRANSLATION_UNIT):
		# Recurse for children of this cursor
		has_children = False;
		for c in cursor.get_children():
			if (not has_children):
				if output:
					print indent(level) + '{'
				has_children = True
			output_cursor_and_children(c, level+1)

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

i=0
while i < len(addition):
	struct = addition[i]
	
	output_cursor_and_children(tu.cursor)
	
	i += 1
print(addition)
