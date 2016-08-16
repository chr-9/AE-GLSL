// https://github.com/baku89/TestPattern/blob/master/bin/data/pixel.frag

uniform sampler2D videoTexture;
uniform vec4 color;

void main( void )
{
	vec4 texColor = texture2D( videoTexture, gl_TexCoord[0].xy );
	
	vec2 pos = gl_FragCoord.xy - vec2(0.5);

	gl_FragColor = mix(texColor, color, 1.0 - clamp(mod(pos.x, 4.0) + mod(pos.y, 4.0), 0.0, 1.0));
}