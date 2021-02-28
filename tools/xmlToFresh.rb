#!/usr/bin/ruby -w

xml = ARGF.read

# Get rid of xml header line.
xml.gsub!( /<\?xml version="1.0" encoding="UTF-8"\s*\?>\n*/, '' )

# Replace single-line comments.
xml.gsub!( /<!--(.*)-->/, '//\1' )

# Replace multi-line comments.
xml.gsub!( /<!--((.|\n)*)-->/, '/*\1*/' )

# Break up "tight" elements
xml.gsub!( /<\/(.+?)><(.+?)>/, "</\\1>\n<\\2>" )
xml.gsub!( /<(.+?)><(.+?)>/, "<\\1>\n<\\2>" )

# Get rid of <objects> element.
xml.gsub!( /<objects>.*\n/, '' )
xml.gsub!( /<\/objects>/, '' )
xml.gsub!( /<\/?objects (root=".*?")>.*\n/, '// TODO had root attribute: \1' )

# Convert include directives
xml.gsub!( /^[ \t]*<include url="(.+?).xml"(.*?)\/>/, 'include url="\1.fresh"/>' )
xml.gsub!( /include url="(.+?)"\/>/, 'include "\1"' )

# Convert const directives
xml.gsub!( /<const type="(.+?)" name="(.+?)">(.+?)<\/.+?>/, 'const \1 \2 \3' )

# Convert object directives
xml.gsub!( /^[ \t]*<object class="(.+?)" name="(.+?)">[ \t]*\n?/, "object \\1 \\2 {\n" )
xml.gsub!( /^[ \t]*<object class="(.+?)" name="(.+?)"[ \t]*\/>/, "object \\1 \\2 {\n}" )
xml.gsub!( /^[ \t]*<object class="(.+?)">[ \t]*\n?/, "object \\1 {\n" )
xml.gsub!( /^[ \t]*<object class="(.+?)">[ \t]*\/>/, "object \\1 {\n}" )
xml.gsub!( /^[ \t]*<\/object>/, '}' )

# Convert class directives
xml.gsub!( /^[ \t]*<class name="(.+?)" extends="(.+?)">[ \t]*\n?/, "class \\1 extends \\2 {\n" )
xml.gsub!( /^[ \t]*<\/class>/, '}' )

#
# Convert properties
#

# Specific properties
xml.gsub!( /^[ \t]*<children>(.*?)\n?/, "children [\n" )
xml.gsub!( /^[ \t]*<\/children>(.*?)\n?/, "]\n" )

xml.gsub!( /^[ \t]*<passthrough>(.*?)\n?/, "passthrough [\n" )
xml.gsub!( /^[ \t]*<\/passthrough>(.*?)\n?/, "]\n" )

xml.gsub!( /^[ \t]*<keyframe>(.*?)\n?/, "object keyframe {\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe \/>(.*?)\n?/, "object keyframe {\n}\n" )
xml.gsub!( /^[ \t]*<keyframe rel-s="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\nrel_s \"\\1\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe rel-t="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\nrel_t \"\\1\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe t="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\nt \"\\1\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe s="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\ns \"\\1\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe label="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\nlabel \"\\1\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<keyframe label="(.+?)" replay-children="(.+?)">(.*?)\n?/, "\t\tobject keyframe {\nlabel \"\\1\"\nreplay_children \"\\2\"\nchildren [\n" )
xml.gsub!( /^[ \t]*<\/keyframe>(.*?)\n?/, "]\n}\n" )

xml.gsub!( /^[ \t]*<child name="(.+?)" \/>/, "object child \"\\1\" {\n}" )
xml.gsub!( /^[ \t]*<child name="(.+?)">/, "object child \"\\1\" {\n" )
xml.gsub!( /<\/child>/, "}" )

xml.gsub!( /^[ \t]*<tween type="(.+?)" \/>/, "tween object \\1 {\n}" )
xml.gsub!( /<\/child>/, "}" )

# Rearrange tween objects to be outside of keyframe children.
xml.gsub!( /children \[((.|\n)+?)tween object (.+?) {\n\s*}\n\s*\]/, "children [\\1]\ntween object \\3 {\n}\n" )

# Normal properties
xml.gsub!( /^[ \t]*<(.+?)>(.*?)<\/.+?>[ \t]*\n?/, "\t\\1 \"\\2\"\n" )

# Multi-line properties
xml.gsub!( /^[ \t]*<(.+?)>((.|\n)*?)<\/.+?>/, "\\1 << EOF\n\\2EOF" )

# Convert empty properties
xml.gsub!( /^<(.+?)><\/(.+?)>?/, "\\1 \"\"" )
xml.gsub!( /^[ \t]*<(.+?) \/>[ \t]*\n?/, "\\1 \"\"" )

#
# Re-indent and print everything.
#

# Strip out all leading spaces.
xml.gsub!( /^[ \t]+/, '' )

indent = 0
leadingSpace = true

xml.each_char do |c|
	if c == ?\n
		leadingSpace = true
		print c
	elsif c == ?\t or c == 32
		if not leadingSpace
			print c
		end
	else		
		if c == ?{ or c == ?[
			indent = indent + 1
		elsif c == ?} or c == ?]
			indent = indent - 1
		end

		if leadingSpace
			leadingSpace = false
			if indent < 0
				indent = 0
				print "INDENT PROBLEM TODO>"
			end
			print "\t" * indent
		end
		print c
	end
end



