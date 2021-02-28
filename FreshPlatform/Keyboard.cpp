//
//  Keyboard.cpp
//  Fresh
//
//  Created by Jeff Wofford on 3/21/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Keyboard.h"

namespace
{
	bool g_keysDown[ fr::Keyboard::MAX_KEYS ] = { false };
}

namespace fr
{

	bool Keyboard::isKeyDown( Key key )
	{
		return g_keysDown[ key ];
	}
	
	void Keyboard::onKeyStateChanged( Key key, bool newState )
	{
		g_keysDown[ key ] = newState;
	}
	
	// TODO
	
	/*
	 A	65	65
	 B	66	66
	 C	67	67
	 D	68	68
	 E	69	69
	 F	70	70
	 G	71	71
	 H	72	72
	 I	73	73
	 J	74	74
	 K	75	75
	 L	76	76
	 M	77	77
	 N	78	78
	 O	79	79
	 P	80	80
	 Q	81	81
	 R	82	82
	 S	83	83
	 T	84	84
	 U	85	85
	 V	86	86
	 W	87	87
	 X	88	88
	 Y	89	89
	 Z	90	90
	 0	48	48
	 1	49	49
	 2	50	50
	 3	51	51
	 4	52	52
	 5	53	53
	 6	54	54
	 7	55	55
	 8	56	56
	 9	57	57
	 a	65	97
	 b	66	98
	 c	67	99
	 d	68	100
	 e	69	101
	 f	70	102
	 g	71	103
	 h	72	104
	 i	73	105
	 j	74	106
	 k	75	107
	 l	76	108
	 m	77	109
	 n	78	110
	 o	79	111
	 p	80	112
	 q	81	113
	 r	82	114
	 s	83	115
	 t	84	116
	 u	85	117
	 v	86	118
	 w	87	119
	 x	88	120
	 y	89	121
	 z	90	122
	 Numpad 0	96	48
	 Numpad 1	97	49
	 Numpad 2	98	50
	 Numpad 3	99	51
	 Numpad 4	100	52
	 Numpad 5	101	53
	 Numpad 6	102	54
	 Numpad 7	103	55
	 Numpad 8	104	56
	 Numpad 9	105	57
	 Multiply	106	42
	 Add	107	43
	 Enter	13	13
	 Subtract	109	45
	 Decimal	110	46
	 Divide	111	47
	 F1	112	0
	 F2	113	0
	 F3	114	0
	 F4	115	0
	 F5	116	0
	 F6	117	0
	 F7	118	0
	 F8	119	0
	 F9	120	0
	 F10	121	0
	 F11	122	0
	 F12	123	0
	 F13	124	0
	 F14	125	0
	 F15	126	0
	 Backspace	8	8
	 Tab	9	9
	 Enter	13	13
	 Shift	16	0
	 Control	17	0
	 Caps Lock	20	0
	 Esc	27	27
	 Spacebar	32	32
	 Page Up	33	0
	 Page Down	34	0
	 End	35	0
	 Home	36	0
	 Left Arrow	37	0
	 Up Arrow	38	0
	 Right Arrow	39	0
	 Down Arrow	40	0
	 Insert	45	0
	 Delete	46	127
	 Num Lock	144	0
	 ScrLk	145	0
	 Pause/Break	19	0
	 ; :	186	59
	 = +	187	61
	 "- _"	189	45
	 / ?	191	47
	 ` ~	192	96
	 [ {	219	91
	 \ |	220	92
	 ] }	221	93
	 " '	222	39
	 ,	188	44
	 .	190	46
	 /	191	47
	 
	 */
	
}

