#!/usr/bin/env ruby -w

# Get the hex number

hex = ARGV[ 0 ]

if !hex
	abort
end

# Remove 0x if it's prepended

if hex[0..1].casecmp( "0x" ) == 0
	hex = hex[2..-1]
	print '0x'
elsif hex[0] == '#'
	hex = hex[1..-1]
	print '#'
end
	

# For each pair, convert to float in range [0,1]

while !hex.empty? do
	byte = hex[0,2];

	term = byte.to_i(16) / 255.0

	adjusted = (( term * term ) * 255 ).round.to_i.to_s( 16 ).rjust( 2, '0' )

	print adjusted

	hex = hex[2..-1];
end

puts
