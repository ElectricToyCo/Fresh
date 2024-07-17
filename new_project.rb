#!/usr/bin/env ruby -w

# Copies and configures the FreshTemplate folder to a new project folder.
# Must be run from the "Fresh" directory, such that FreshTemplate is an immediate subdirectory.

require 'fileutils'
require 'pathname'
require 'find'

def yesno
  begin
    system("stty raw -echo")
    str = STDIN.getc
  ensure
    system("stty -raw echo")
  end
  if str == "y"
    return true
  elsif str == "n"
    return false
  else
    raise "Invalid character."
  end
end

def prompt( message, default = '' )
	puts message + (( not default.empty? ) ? " (Default: #{default})" : '' )
	answer = gets.chomp.strip
	answer = answer.empty? ? default : answer
	
	if answer.empty?
		abort "No answer provided. Aborting."
	end

	answer
end

def replace( path, from, to )
	text = File.read(path)
	new_contents = text.gsub( from, to )
	File.open( path, "w" ) { |file| file.puts new_contents }
end

def rename( path, from, to )
	newPath = path.gsub( from, to )
	if not newPath == path
		FileUtils.mv path, newPath
	end
end

puts "Create a Fresh App Project"
puts "=========================="
puts "This script creates a new app project that uses the Fresh game engine."

# Make sure we're in the Fresh directory.

currentFolder = Pathname.pwd

if not currentFolder.basename.to_s == 'Fresh'
	abort 'ERROR: You must run this script from the Fresh directory. Aborting.'
end

# Make sure we see a ProjectTemplate subfolder

templateFolder = Pathname.new 'ProjectTemplate'

if not templateFolder.exist? or not templateFolder.directory?
	abort 'ERROR: Could not find the Fresh/ProjectTemplate subdirectory. Aborting.'
end

# Get information from the user about the project

puts "A few questions to help configure your project..."

company = prompt( "What's your company name? This is used for your subfolder in the user's Documents directory, so it should be a valid directory name.", 'The Electric Toy Co' )

package = prompt( "What's your package domain? This is used by Android, Mac, and iOS as a sort of namespace for your app. It's normally in reverse-DNS order, so if your company was apple.com, your package domain would be com.apple.", 'co.electrictoy' )

if not package =~ /\A[a-z\.]+\z/
	abort 'PROBLEM: The package domain must contain only letters and dots. Aborting.'
end

title = prompt "What's your app's title? This is the full, long name, like what would be shown in the app store, or on a window caption. (Example: \"House of Shadows\")"

shortName = prompt "What's a short name for your app? This is used a lot: for the app folder name, the package name, certain project names. It should be short, and can only contain alphanumeric characters."

if not shortName =~ /\A\p{Alnum}+\z/
	abort 'PROBLEM: The short name must contain alphanumeric characters only. Aborting.'
end

namespace = prompt "What namespace will you use? This plays two roles: it acts as the C++ namespace for your code, and also as the prefix for Objective-C classes. Must people like them very short. (Example: \"dig\")"

if not namespace =~ /\A[a-zA-Z][a-zA-Z0-9_]+\z/
	abort 'PROBLEM: The namespace must be a valid C++ identifier with no leading underscore. Aborting.'
end

while true
	parent = prompt "What existing folder should be the parent for the project folder? Enter a valid existing folder path."

	parent = File.expand_path parent

	if File.directory? parent
		destinationFolder = File.join( parent, shortName )
		break
	else
		puts "Invalid folder. What now, what now?"
	end
end

puts "You entered:"
puts "------------"
puts "Company:        '#{company}'"
puts "Package Domain: '#{package}'"
puts "Title:          '#{title}'"
puts "Short Name:     '#{shortName}'"
puts "Namespace:      '#{namespace}'"
puts "Location:       '#{parent}/#{shortName}"
puts
print "Ready to create the project? [y/N] > "

begin
	result = yesno
rescue
	result = false
end

puts

if not result
	abort 'Aborting.'
end

# Copy the template folder to a new directory in the parent.

puts "Creating project in folder #{destinationFolder.to_s}"

# Make sure there's not already something there.

if File.exist? destinationFolder
	abort 'A file or folder of that name already exists in that location.'
end

FileUtils.cp_r templateFolder, destinationFolder

# Replace-in-files throughout the new folder.

FileUtils.cd destinationFolder

if true		# False only when you want to keep stock names. For development of this script.
	begin

		Find.find( '.' ) do |path|

			# Replace strings in files.

			if not File.directory? path and not path =~ /.*\.DS_Store\z/ and not path =~ /.+\.png\z/ and not path =~ /.+\.jar\z/
				begin
					replace( path, 'FRESH COMPANY NAME', company )
					replace( path, 'fresh.package.domain', package )
					replace( path, 'FRESH APP TITLE', title )
					replace( path, 'freshshortname', shortName )
					replace( path, 'fresh_namespace', namespace )

				rescue Exception => e
					puts "WARNING: Exception while processing file #{path}: #{e.message}. Ignoring."
				end
			end
		end

		Find.find( '.' ) do |path|

			# Rename files and directories.
			begin
				rename( path, 'FRESH COMPANY NAME', company )
				rename( path, 'fresh.package.domain', package )
				rename( path, 'FRESH APP TITLE', title )
				rename( path, 'freshshortname', shortName )
				rename( path, 'fresh_namespace', namespace )
			rescue Exception => e
				puts "WARNING: Exception while moving path #{path}: #{e.message}. Ignoring."
			end
		end


	rescue Exception => e

		abort "ERROR: Exception while processing project: #{e.message}"

	end
end

# Initialize git and the Fresh submodule.
`git init`
`git submodule add ssh://jeffwoff@mi3-ss117.a2hosting.com:7822/home/jeffwoff/repos/git/Fresh.git`
`git add -A`
`git commit -m "First post."`

puts "Done. Your project is at #{destinationFolder.to_s}"
