//
//  Lens.fsh
//  Fresh
//
//  Created by Jeff Wofford on 11/22/2011.
//  Copyright jeffwofford.com 2011. All rights reserved.
//

uniform sampler2D diffuseTexture;

varying lowp vec4 frag_texCoord;

void main()
{
	lowp vec2 texCoord = 2.0 * ( frag_texCoord.st - 0.5 );
	
	texCoord *= texCoord * texCoord;			// Cubed
	texCoord += 1.0;
	
	gl_FragColor = texture2D( diffuseTexture, mix( frag_texCoord.st, texCoord * 0.5, 0.5 ));
}

//void main()
//{
//	// Adapted from http://www.francois-tarlier.com/blog/cubic-lens-distortion-shader/
//	
//	// lens distortion coefficient (between [sic]
//	lowp float k = -0.0;
//
//	// cubic distortion value
//	lowp float kcube = 0.5;
//
//	lowp float r2 = (frag_texCoord.x-0.5) * (frag_texCoord.x-0.5) + (frag_texCoord.y-0.5) * (frag_texCoord.y-0.5);
//	lowp float f = 1.0 + r2 * (k + kcube * sqrt( r2 ));
//
//	// get the right pixel for the current position
//	lowp float x = f*(frag_texCoord.x-0.5)+0.5;
//	lowp float y = f*(frag_texCoord.y-0.5)+0.5;
//
//
//	gl_FragColor = texture2D( diffuseTexture, vec2( x, y ));
//}
