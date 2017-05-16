uniform vec4 colorPositif;
uniform vec4 colorNegatif;
uniform vec4 colorNeutre;
uniform int invertPolarity;

varying vec3 normal;
varying vec3 vertex_to_light_vector;
varying vec4 color;

void main()
{
	// Transforming The Vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Transforming The Normal To ModelView-Space
	normal = gl_NormalMatrix * gl_Normal; 

	// Transforming The Vertex Position To ModelView-Space
	vec4 vertex_in_modelview_space = gl_ModelViewMatrix * gl_Vertex;

	// Calculating The Vector From The Vertex Position To The Light Position
	vertex_to_light_vector = vec3(gl_LightSource[0].position - vertex_in_modelview_space);

	vertex_to_light_vector = gl_LightSource[0].position;

	color = gl_Color;
		
	//Positif
	if(gl_Color.r < 1.0)
	{
		if(invertPolarity == 0)
			color = colorPositif * (1-gl_Color.r) + colorNeutre * gl_Color.r;
		else
			color = colorNegatif * (1-gl_Color.r) + colorNeutre * gl_Color.r;
	}

	//Negatif
	if(gl_Color.b < 1.0)
	{
		if(invertPolarity == 0)
			color = colorNegatif * (1-gl_Color.b) + colorNeutre * gl_Color.b;
		else
			color = colorPositif * (1-gl_Color.b) + colorNeutre * gl_Color.b;
	}

	color.a = gl_Color.a;
	
}