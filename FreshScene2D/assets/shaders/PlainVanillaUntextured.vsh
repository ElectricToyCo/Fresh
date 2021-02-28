//
//  PlainVanillaUntextured.vsh
//  Fresh
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

attribute highp vec2 position;
attribute lowp vec4 color;

varying lowp vec4 vertex_color;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 0.0, 1.0 );
	vertex_color = color;
}
