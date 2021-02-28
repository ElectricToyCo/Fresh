//
//  PlainVanilla.fsh
//  FreshScene2D
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//
uniform sampler2D diffuseTexture;
uniform lowp vec4 color_multiply;
uniform lowp vec4 color_additive;

varying highp vec2 fragment_texCoord;

void main()
{
	lowp vec4 color = color_multiply * texture2D( diffuseTexture, fragment_texCoord );

	/* To support additive alpha:

	 color = color * ( color.a + color_additive.a ) / color.a;
	 color_additive.a = 0.0;
	 
	 */

	gl_FragColor = color + color.a * color_additive;
}
