//
//  PlainVanillaUntextured.vsh
//  Fresh
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

attribute vec2 position;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 0.0, 1.0 );
}
