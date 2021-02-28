//
//  PlainVanillaUntextured.fsh
//  FreshScene2D
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright jeffwofford.com 2010. All rights reserved.
//
uniform lowp vec4 color_multiply;
uniform lowp vec4 color_additive;

varying lowp vec4 vertex_color;

void main()
{
	lowp vec4 color = vertex_color * color_multiply;
	
	/* To support additive alpha:
	 
	 color = color * ( color.a + color_additive.a ) / color.a;
	 color_additive.a = 0.0;
	 
	 */
	
	gl_FragColor = color + color.a * color_additive;
}
