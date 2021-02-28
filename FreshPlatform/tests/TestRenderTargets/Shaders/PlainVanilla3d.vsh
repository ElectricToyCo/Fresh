//
//  PlainVanilla.vsh
//  ExperimentShading
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

attribute vec3 position;
attribute vec2 texCoord;
attribute vec4 color;

varying lowp vec4 frag_texCoord;
varying lowp vec4 frag_color;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );
	
	frag_texCoord = textureMatrix * vec4( texCoord, 0.0, 1.0 );
	frag_color = color;
}
