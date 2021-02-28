#!/usr/bin/ruby -w

# Converts Fresh-format object XML files to JSON files

require 'optparse'
require 'rexml/document'
require 'json'
include REXML
require_relative 'indent_string'

$VERBOSE = true
$INFILE = $stdin
$OUTFILE = $stdout

def verbose message
	if $VERBOSE
		puts message
	end
end

def escape_string value

	if value == nil
		""
	else
		value.gsub( '"', '\\\"' )
	end
end

def create_array element, depth

	$OUTFILE.puts
	$OUTFILE.puts '['.indent( depth )

	# Child elements
	first = true
	element.each_element do |child|
		if not first then $OUTFILE.puts ',' end
		create_object( child, depth + 1 )
		first = false
	end

	$OUTFILE.puts
	$OUTFILE.print ']'.indent( depth )
end

def create_hash element, depth

	if element.elements.count > 0 or element.attributes.count > 0
		
		$OUTFILE.puts
		$OUTFILE.puts '{'.indent( depth )

		# Comments
		element.comments.each do |comment|
			$OUTFILE.puts "//#{comment}".indent( depth + 1 )
		end

		# Node name
		$OUTFILE.puts "\"@node\" : \"#{element.name}\",".indent( depth + 1 )

		# Child attributes
		first = true
		element.attributes.each do |name, value|
			if not first then $OUTFILE.puts ',' end
			$OUTFILE.print "\".#{name}\" : \"#{escape_string value}\"".indent( depth + 1 )
			first = false
		end

		# Child elements
		element.each_element do |child|
			if not first then $OUTFILE.puts ',' end
			$OUTFILE.print "\"#{child.name}\" : ".indent( depth + 1 )
			create_object( child, depth + 1 )
			first = false
		end

		# Raw text, if any

		text = element.text

		text = text.nil? ? "" : text.strip

		if not text.nil? and text.length > 0
			$OUTFILE.print first ? "" : ",\n"
			$OUTFILE.puts "\"value\" : \"#{text}\"".indent( depth + 1 )
		end

		# TODO Comments

		$OUTFILE.puts
		$OUTFILE.print '}'.indent( depth )
	else

		# Value
		$OUTFILE.print "\"#{escape_string element.text}\""

	end

end
def create_object element, depth

	# Special cases for known array elements

	if [ 'objects', 'children', 'passthrough' ].include? element.name
		create_array element, depth
	else
		create_hash element, depth
	end
end

OptionParser.new do |opts|
	opts.banner = "Converts Fresh-format object XML files to JSON files.\nUsage: xml2json.rb [options]"

	opts.on( "-i", "--input PATH", "Input file" ) do |opt|
		verbose "Reading from #{opt}."
		$INFILE = File.open( opt, 'r' )
	end

	opts.on( "-o", "--output PATH", "Output file" ) do |opt|
		verbose "Writing to #{opt}."
		$OUTFILE = File.open( opt, 'w' )
	end

	opts.on( "-v", "--[no-]verbose", "$OUTFILE.print extra output" ) do |opt|
		$VERBOSE = opt
	end

	opts.on_tail( "-h", "--help", "Show this message" ) do
		$OUTFILE.puts opts
		exit
	end

end.parse!

xml_file = Document.new $INFILE.read

create_object( xml_file.root, 0 )