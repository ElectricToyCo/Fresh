#!/usr/bin/env ruby -w

require 'optparse'
require 'rexml/document'
include REXML

$VERBOSE = true
$INFILE = $stdin
$OUTFILE = $stdout
$MC_CLASS = 'TODO'
$MC_TEXTURE = ''
$LABEL_KEYFRAMES = true

def verbose message
	if $VERBOSE
		puts message
	end
end

OptionParser.new do |opts|
	opts.banner = "Converts TexturePacker atlas XML data to a Fresh MovieClip class.\nUsage: tp2mc.rb [options]"

	opts.on( "-i", "--input PATH", "Input file" ) do |opt|
		verbose "Reading from #{opt}."
		$INFILE = File.open( opt, 'r' )
	end

	opts.on( "-o", "--output PATH", "Output file" ) do |opt|
		verbose "Writing to #{opt}."
		$OUTFILE = File.create( opt, 'w' )
	end

	opts.on( '-c', '--class CLASS_NAME', 'Output class name') do |opt|
		verbose "Using output class name '#{opt}"
		$MC_CLASS = opt;
	end

	opts.on( '-t', '--texture TEXTURE_NAME', 'Output texture name') do |opt|
		verbose "Using output texture name '#{opt}"
		$MC_TEXTURE = opt;
	end

	opts.on( "-v", "--[no-]verbose", "Print extra output" ) do |opt|
		$VERBOSE = opt
	end

	opts.on( '-l', '--[no-]labels', 'Added labels to keyframes based on the original file name. Default: on.' ) do |opt|
		$LABEL_KEYFRAMES = opt
	end

	opts.on_tail( "-h", "--help", "Show this message" ) do
		puts opts
		exit
	end

end.parse!

doc = Document.new $INFILE.read

if doc.root.name != 'TextureAtlas'
	abort "XML document with root '${doc.root.name}' doesn't appear to be a TexturePacker atlas file."
end

originalImagePath = doc.root.attributes['imagePath']
imageWidth = doc.root.attributes['width'].to_i
imageHeight = doc.root.attributes['height'].to_i

verbose "Image with original path '#{originalImagePath}' @ resolution #{imageWidth},#{imageHeight}"

if $MC_TEXTURE.empty?
	$MC_TEXTURE = originalImagePath
end

outMovieClip = %[<class name="#{$MC_CLASS}" extends="MovieClip">\n\t<passthrough>\n]

doc.root.each_element do |element|
	if element.name != 'sprite'
		puts "Found unrecognized element '#{element.name}'. Skipping."
	else
		frameName = element.attributes['n']

		frameLeftPx = element.attributes['x'].to_i				
		frameTopPx = element.attributes['y'].to_i				
		frameWidthPx = element.attributes['w'].to_i				
		frameHeightPx = element.attributes['h'].to_i			
		frameRightPx = frameLeftPx + frameWidthPx				
		frameBottomPx = frameTopPx + frameHeightPx				

		if element.attributes.include? 'oX'
			frameLeftOffsetPx = element.attributes['oX'].to_i	
			frameTopOffsetPx = element.attributes['oY'].to_i	
			frameOriginalWidthPx = element.attributes['oW'].to_i
			frameOriginalHeightPx = element.attributes['oH'].to_i 

			frameWidthOffsetPx = frameWidthPx - frameOriginalWidthPx		# Typically negative
			frameHeightOffsetPx = frameHeightPx - frameOriginalHeightPx		

			frameRightOffsetPx = frameLeftOffsetPx + frameWidthOffsetPx		
			frameBottomOffsetPx = frameTopOffsetPx + frameHeightOffsetPx	

			centerOffsetX = ( frameLeftOffsetPx + frameRightOffsetPx ) * 0.5
			centerOffsetY = ( frameTopOffsetPx + frameBottomOffsetPx ) * 0.5

			pivotX = -centerOffsetX
			pivotY = -centerOffsetY
		end

		frameXNormalized = frameLeftPx.to_f / imageWidth;
		frameYNormalized = frameTopPx.to_f / imageHeight;
		frameWidthNormalized = frameWidthPx.to_f / imageWidth;
		frameHeightNormalized = frameHeightPx.to_f / imageHeight;

		originalCenterX = frameXNormalized + frameWidthNormalized;
		originalCenterY = frameYNormalized + frameHeightNormalized;

		frameRightNormalized = frameXNormalized + frameWidthNormalized
		frameBottomNormalized = frameYNormalized + frameHeightNormalized

		rect = %[#{frameXNormalized},#{frameYNormalized},#{frameRightNormalized},#{frameBottomNormalized}]

		verbose "Frame '#{frameName}' with pixel rect (#{frameLeftPx},#{frameTopPx},#{frameWidthPx},#{frameHeightPx}) normalized: (#{rect})"

		outMovieClip << %[\t\t<keyframe];
		if $LABEL_KEYFRAMES
			outMovieClip << %[ label='#{frameName}']
		end
		outMovieClip << %[>\n]
		outMovieClip << %[\t\t\t<texture>Texture'#{$MC_TEXTURE}'</texture>\n]
		outMovieClip <<	%[\t\t\t<textureWindow>(#{rect})</textureWindow>\n]

		if pivotX
			outMovieClip << %[\t\t\t<pivot>(#{pivotX},#{pivotY})</pivot>\n]
		end
		outMovieClip << %[\t\t</keyframe>\n]
	end
end

outMovieClip << %[\t</passthrough>\n</class>\n]

$OUTFILE.write outMovieClip