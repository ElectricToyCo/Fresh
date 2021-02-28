//
//  PlainVanilla3d.fsh
//  ExperimentShading
//
//  Created by Jeff Wofford on 11/16/2011.
//  Copyright jeffwofford.com 2011. All rights reserved.
//

uniform sampler2D diffuseTexture;
uniform lowp vec4 material_color;

varying lowp vec4 frag_texCoord;
varying lowp vec4 frag_color;

void main()
{
	gl_FragColor = material_color * frag_color * texture2D( diffuseTexture, frag_texCoord.st );
}
