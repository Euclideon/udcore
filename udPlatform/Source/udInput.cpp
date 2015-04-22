#include "udInput_Internal.h"

const int gMaxDevices[udID_Max] =
{
  MAX_KEYBOARDS, // udID_Keyboard
  MAX_MOUSES, // udID_Mouse
  MAX_GAMEPADS, // udID_Gamepad
//  1, // udID_TouchScreen
//  1, // udID_Accelerometer
//  1  // udID_Compas
};

//static InputState liveState;
InputState gInputState[2];
int gCurrentInputState = 0;

static unsigned int mouseLock = 0;
static unsigned int mouseLocked = 0;

static int ignoreMouseEvents = 0;


// ********************************************************
// Author: Manu Evans, March 2015
udInputDeviceState udInput_GetDeviceState(udInputDevice, int)
{
  return udIDS_Ready;
}

// ********************************************************
// Author: Manu Evans, March 2015
unsigned int udInput_LockMouseOnButtons(unsigned int buttonBits)
{
  unsigned int old = mouseLock;
  mouseLock = buttonBits;
  return old;
}

// ********************************************************
// Author: Manu Evans, March 2015
bool udInput_WasPressed(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    for (int i=0; i<gMaxDevices[device]; ++i)
      if(udInput_WasPressed(device, control, i))
        return true;
    return false;
  }

  InputState prev = gInputState[1-gCurrentInputState];
  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return state.keys[deviceIndex][control] && !prev.keys[deviceIndex][control];
    case udID_Mouse:
      return state.mouse[deviceIndex][control] && !prev.mouse[deviceIndex][control];;
    case udID_Gamepad:
      return state.gamepad[deviceIndex][control] && !prev.gamepad[deviceIndex][control];
  }
  return false;
}

// ********************************************************
// Author: Manu Evans, March 2015
bool udInput_WasReleased(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    for (int i=0; i<gMaxDevices[device]; ++i)
      if(udInput_WasReleased(device, control, i))
        return true;
    return false;
  }

  InputState prev = gInputState[1-gCurrentInputState];
  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return !state.keys[deviceIndex][control] && prev.keys[deviceIndex][control];
    case udID_Mouse:
      return !state.mouse[deviceIndex][control] && prev.mouse[deviceIndex][control];
    case udID_Gamepad:
      return !state.gamepad[deviceIndex][control] && prev.gamepad[deviceIndex][control];
  }
  return false;
}

// ********************************************************
// Author: Manu Evans, March 2015
float udInput_State(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    float state = 0.f;
    for (int i=0; i<gMaxDevices[device]; ++i)
      state += udInput_State(device, control, i);
    return state;
  }

  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return state.keys[deviceIndex][control] ? 1.f : 0.f;
    case udID_Mouse:
      return state.mouse[deviceIndex][control];
    case udID_Gamepad:
      return state.gamepad[deviceIndex][control];
//    case udID_TouchScreen:
//    case udID_Accelerometer:
//    case udID_Compas:
      // etc...
  }
  return 0.0;
}

// ********************************************************
// Author: Manu Evans, March 2015
void udInput_Init()
{
  udInput_InitInternal();
}

// ********************************************************
// Author: Manu Evans, March 2015
void udInput_Update()
{
  // switch input frame
  gCurrentInputState = 1 - gCurrentInputState;

  udInput_UpdateInternal();
}


unsigned char udAsciiToUDKey[128] =
{
  udKC_Unknown, // 0	000	00	00000000	NUL	&#000;	 	Null char
  udKC_Unknown, // 1	001	01	00000001	SOH	&#001;	 	Start of Heading
  udKC_Unknown, // 2	002	02	00000010	STX	&#002;	 	Start of Text
  udKC_Unknown, // 3	003	03	00000011	ETX	&#003;	 	End of Text
  udKC_Unknown, // 4	004	04	00000100	EOT	&#004;	 	End of Transmission
  udKC_Unknown, // 5	005	05	00000101	ENQ	&#005;	 	Enquiry
  udKC_Unknown, // 6	006	06	00000110	ACK	&#006;	 	Acknowledgment
  udKC_Unknown, // 7	007	07	00000111	BEL	&#007;	 	Bell
  udKC_Backspace, // 8	010	08	00001000	BS	&#008;	 	Back Space
  udKC_Tab, // 9	011	09	00001001	HT	&#009;	 	Horizontal Tab
  udKC_Unknown, // 10	012	0A	00001010	LF	&#010;	 	Line Feed
  udKC_Unknown, // 11	013	0B	00001011	VT	&#011;	 	Vertical Tab
  udKC_Unknown, // 12	014	0C	00001100	FF	&#012;	 	Form Feed
  udKC_Enter, // 13	015	0D	00001101	CR	&#013;	 	Carriage Return
  udKC_Unknown, // 14	016	0E	00001110	SO	&#014;	 	Shift Out / X-On
  udKC_Unknown, // 15	017	0F	00001111	SI	&#015;	 	Shift In / X-Off
  udKC_Unknown, // 16	020	10	00010000	DLE	&#016;	 	Data Line Escape
  udKC_Unknown, // 17	021	11	00010001	DC1	&#017;	 	Device Control 1 (oft. XON)
  udKC_Unknown, // 18	022	12	00010010	DC2	&#018;	 	Device Control 2
  udKC_Unknown, // 19	023	13	00010011	DC3	&#019;	 	Device Control 3 (oft. XOFF)
  udKC_Unknown, // 20	024	14	00010100	DC4	&#020;	 	Device Control 4
  udKC_Unknown, // 21	025	15	00010101	NAK	&#021;	 	Negative Acknowledgement
  udKC_Unknown, // 22	026	16	00010110	SYN	&#022;	 	Synchronous Idle
  udKC_Unknown, // 23	027	17	00010111	ETB	&#023;	 	End of Transmit Block
  udKC_Unknown, // 24	030	18	00011000	CAN	&#024;	 	Cancel
  udKC_Unknown, // 25	031	19	00011001	EM	&#025;	 	End of Medium
  udKC_Unknown, // 26	032	1A	00011010	SUB	&#026;	 	Substitute
  udKC_Escape, // 27	033	1B	00011011	ESC	&#027;	 	Escape
  udKC_Unknown, // 28	034	1C	00011100	FS	&#028;	 	File Separator
  udKC_Unknown, // 29	035	1D	00011101	GS	&#029;	 	Group Separator
  udKC_Unknown, // 30	036	1E	00011110	RS	&#030;	 	Record Separator
  udKC_Unknown, // 31	037	1F	00011111	US	&#031;	 	Unit Separator
  udKC_Space, // 32	040	20	00100000	 	&#32;	 	Space
  udKC_Unknown, // 33	041	21	00100001	!	&#33;	 	Exclamation mark
  udKC_Unknown, // 34	042	22	00100010	"	&#34;	&quot;	Double quotes (or speech marks)
  udKC_Unknown, // 35	043	23	00100011	#	&#35;	 	Number
  udKC_Unknown, // 36	044	24	00100100	$	&#36;	 	Dollar
  udKC_Unknown, // 37	045	25	00100101	%	&#37;	 	Procenttecken
  udKC_Unknown, // 38	046	26	00100110	&	&#38;	&amp;	Ampersand
  udKC_Apostrophe, // 39	047	27	00100111	'	&#39;	 	Single quote
  udKC_Unknown, // 40	050	28	00101000	(	&#40;	 	Open parenthesis (or open bracket)
  udKC_Unknown, // 41	051	29	00101001	)	&#41;	 	Close parenthesis (or close bracket)
  udKC_NumpadMultiply, // 42	052	2A	00101010	*	&#42;	 	Asterisk
  udKC_NumpadPlus, // 43	053	2B	00101011	+	&#43;	 	Plus
  udKC_Comma, // 44	054	2C	00101100	,	&#44;	 	Comma
  udKC_Hyphen, // 45	055	2D	00101101	-	&#45;	 	Hyphen
  udKC_Period, // 46	056	2E	00101110	.	&#46;	 	Period, dot or full stop
  udKC_ForwardSlash, // 47	057	2F	00101111	/	&#47;	 	Slash or divide
  udKC_0, // 48	060	30	00110000	0	&#48;	 	Zero
  udKC_1, // 49	061	31	00110001	1	&#49;	 	One
  udKC_2, // 50	062	32	00110010	2	&#50;	 	Two
  udKC_3, // 51	063	33	00110011	3	&#51;	 	Three
  udKC_4, // 52	064	34	00110100	4	&#52;	 	Four
  udKC_5, // 53	065	35	00110101	5	&#53;	 	Five
  udKC_6, // 54	066	36	00110110	6	&#54;	 	Six
  udKC_7, // 55	067	37	00110111	7	&#55;	 	Seven
  udKC_8, // 56	070	38	00111000	8	&#56;	 	Eight
  udKC_9, // 57	071	39	00111001	9	&#57;	 	Nine
  udKC_Unknown, // 58	072	3A	00111010	:	&#58;	 	Colon
  udKC_Semicolon, // 59	073	3B	00111011	;	&#59;	 	Semicolon
  udKC_Unknown, // 60	074	3C	00111100	<	&#60;	&lt;	Less than (or open angled bracket)
  udKC_Equals, // 61	075	3D	00111101	=	&#61;	 	Equals
  udKC_Unknown, // 62	076	3E	00111110	>	&#62;	&gt;	Greater than (or close angled bracket)
  udKC_Unknown, // 63	077	3F	00111111	?	&#63;	 	Question mark
  udKC_Unknown, // 64	100	40	01000000	@	&#64;	 	At symbol
  udKC_A, // 65	101	41	01000001	A	&#65;	 	Uppercase A
  udKC_B, // 66	102	42	01000010	B	&#66;	 	Uppercase B
  udKC_C, // 67	103	43	01000011	C	&#67;	 	Uppercase C
  udKC_D, // 68	104	44	01000100	D	&#68;	 	Uppercase D
  udKC_E, // 69	105	45	01000101	E	&#69;	 	Uppercase E
  udKC_F, // 70	106	46	01000110	F	&#70;	 	Uppercase F
  udKC_G, // 71	107	47	01000111	G	&#71;	 	Uppercase G
  udKC_H, // 72	110	48	01001000	H	&#72;	 	Uppercase H
  udKC_I, // 73	111	49	01001001	I	&#73;	 	Uppercase I
  udKC_J, // 74	112	4A	01001010	J	&#74;	 	Uppercase J
  udKC_K, // 75	113	4B	01001011	K	&#75;	 	Uppercase K
  udKC_L, // 76	114	4C	01001100	L	&#76;	 	Uppercase L
  udKC_M, // 77	115	4D	01001101	M	&#77;	 	Uppercase M
  udKC_N, // 78	116	4E	01001110	N	&#78;	 	Uppercase N
  udKC_O, // 79	117	4F	01001111	O	&#79;	 	Uppercase O
  udKC_P, // 80	120	50	01010000	P	&#80;	 	Uppercase P
  udKC_Q, // 81	121	51	01010001	Q	&#81;	 	Uppercase Q
  udKC_R, // 82	122	52	01010010	R	&#82;	 	Uppercase R
  udKC_S, // 83	123	53	01010011	S	&#83;	 	Uppercase S
  udKC_T, // 84	124	54	01010100	T	&#84;	 	Uppercase T
  udKC_U, // 85	125	55	01010101	U	&#85;	 	Uppercase U
  udKC_V, // 86	126	56	01010110	V	&#86;	 	Uppercase V
  udKC_W, // 87	127	57	01010111	W	&#87;	 	Uppercase W
  udKC_X, // 88	130	58	01011000	X	&#88;	 	Uppercase X
  udKC_Y, // 89	131	59	01011001	Y	&#89;	 	Uppercase Y
  udKC_Z, // 90	132	5A	01011010	Z	&#90;	 	Uppercase Z
  udKC_LeftBracket, // 91	133	5B	01011011	[	&#91;	 	Opening bracket
  udKC_BackSlash, // 92	134	5C	01011100	\	&#92;	 	Backslash
  udKC_RightBracket, // 93	135	5D	01011101	]	&#93;	 	Closing bracket
  udKC_Unknown, // 94	136	5E	01011110	^	&#94;	 	Caret - circumflex
  udKC_Unknown, // 95	137	5F	01011111	_	&#95;	 	Underscore
  udKC_Grave, // 96	140	60	01100000	`	&#96;	 	Grave accent
  udKC_A, // 97	141	61	01100001	a	&#97;	 	Lowercase a
  udKC_B, // 98	142	62	01100010	b	&#98;	 	Lowercase b
  udKC_C, // 99	143	63	01100011	c	&#99;	 	Lowercase c
  udKC_D, // 100	144	64	01100100	d	&#100;	 	Lowercase d
  udKC_E, // 101	145	65	01100101	e	&#101;	 	Lowercase e
  udKC_F, // 102	146	66	01100110	f	&#102;	 	Lowercase f
  udKC_G, // 103	147	67	01100111	g	&#103;	 	Lowercase g
  udKC_H, // 104	150	68	01101000	h	&#104;	 	Lowercase h
  udKC_I, // 105	151	69	01101001	i	&#105;	 	Lowercase i
  udKC_J, // 106	152	6A	01101010	j	&#106;	 	Lowercase j
  udKC_K, // 107	153	6B	01101011	k	&#107;	 	Lowercase k
  udKC_L, // 108	154	6C	01101100	l	&#108;	 	Lowercase l
  udKC_M, // 109	155	6D	01101101	m	&#109;	 	Lowercase m
  udKC_N, // 110	156	6E	01101110	n	&#110;	 	Lowercase n
  udKC_O, // 111	157	6F	01101111	o	&#111;	 	Lowercase o
  udKC_P, // 112	160	70	01110000	p	&#112;	 	Lowercase p
  udKC_Q, // 113	161	71	01110001	q	&#113;	 	Lowercase q
  udKC_R, // 114	162	72	01110010	r	&#114;	 	Lowercase r
  udKC_S, // 115	163	73	01110011	s	&#115;	 	Lowercase s
  udKC_T, // 116	164	74	01110100	t	&#116;	 	Lowercase t
  udKC_U, // 117	165	75	01110101	u	&#117;	 	Lowercase u
  udKC_V, // 118	166	76	01110110	v	&#118;	 	Lowercase v
  udKC_W, // 119	167	77	01110111	w	&#119;	 	Lowercase w
  udKC_X, // 120	170	78	01111000	x	&#120;	 	Lowercase x
  udKC_Y, // 121	171	79	01111001	y	&#121;	 	Lowercase y
  udKC_Z, // 122	172	7A	01111010	z	&#122;	 	Lowercase z
  udKC_Unknown, // 123	173	7B	01111011	{	&#123;	 	Opening brace
  udKC_Unknown, // 124	174	7C	01111100	|	&#124;	 	Vertical bar
  udKC_Unknown, // 125	175	7D	01111101	}	&#125;	 	Closing brace
  udKC_Unknown, // 126	176	7E	01111110	~	&#126;	 	Equivalency sign - tilde
  udKC_Delete // 127	177	7F	01111111		&#127;	 	Delete
};
