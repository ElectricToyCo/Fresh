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

attribute vec2 position;
attribute vec2 texCoord;

varying lowp vec4 fragment_texCoord;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 0.0, 1.0 );
	
	fragment_texCoord = textureMatrix * vec4( texCoord, 0.0, 1.0 );
}
