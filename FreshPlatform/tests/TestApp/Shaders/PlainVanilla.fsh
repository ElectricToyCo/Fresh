//
//  PlainVanilla.fsh
//  ExperimentShading
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//

uniform sampler2D diffuseTexture;
uniform lowp vec4 material_color;

varying lowp vec4 fragment_texCoord;

void main()
{
	gl_FragColor = material_color * texture2D( diffuseTexture, fragment_texCoord.st );
}
