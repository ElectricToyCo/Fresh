#!/usr/bin/env ruby -w

# upver - Updates project version, build, and SCM revision information.

require 'date'
require 'pathname'
require 'fileutils'

### Parse arguments

if ARGV.length < 1
	puts %Q{Usage: upver <in-file> [out-file]"

Run this program from within your project SCM working space.
Reads former build information from *in-file* (required).
Writes new build information to *out-file*.
If *out-file* is not provided, overwrites *in-file*.
If *in-file* is not provided, initializes it to a new file identifying version 1.0.}
	
	exit
end

inFile = outFile = ARGV[ 0 ]

if ARGV.length > 1
	outFile = ARGV[ 1 ]
end

### Read input file

if not File.file? inFile
	puts "Could not open input file '#{inFile}'. Resetting at version 1.0."

	inFilePath = Pathname.new inFile

	if not inFilePath.parent.exist?
		FileUtils.mkdir inFilePath.parent
	end

	File.open( inFile, 'w' ) do |file|
		file.puts "1.0"
	end
end

lines = IO.readlines inFile

if lines.length < 1
	puts "Input file '#{inFile}' had #{lines.length} lines instead of 1-4 lines expected. Aborting."
	exit
end

formerBuildNumber = 0
if lines.length >= 2 
	formerBuildNumber = lines[ 1 ].to_i
end

# Convert path to proper format, if needed.
outFile = outFile.gsub('\\','/').gsub(/^\//,'//').gsub( /([a-zA-Z]):\//, '/cygdrive/\1/' )

### Write the output file

FileUtils.mkdir_p( File.dirname( outFile ))
File.open( outFile, 'w' ) do |file|
	file.puts lines[ 0 ]
	file.puts formerBuildNumber + 1
	file.puts DateTime.now.strftime '%Y-%m-%d %H:%M:%S'
	file.puts `git rev-parse HEAD`[0...8]
	file.fsync
end
