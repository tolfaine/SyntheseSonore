uniform sampler2D Texture0;
uniform sampler2D Texture1;
uniform float screen_width;
uniform float screen_height;



float LinearizeDepth(float z)
{
	/*float f = 400.0;
	float depth =  1 / (f - zoverw*f);
	if(depth > 1)
		depth = 1;
	return depth;*/

	float n = 1.0; // camera z near
  	float f = 2000.0; // camera z far
  	return (2.0 * n) / (f + n - z * (f - n));

}

void main (void)
{
	float xstep = 1.0/screen_width;
	float ystep = 1.0/screen_height;
	float ratio = screen_width / screen_height;

	vec4 color = texture2D( Texture0 , vec2( gl_TexCoord[0] ) );
	float depth = texture2D( Texture1 , vec2( gl_TexCoord[0] ) ).r;	
	
	//Permet de scaler la profondeur
	depth = LinearizeDepth(depth);
	
	//On calcule la luminance (approximation pour cell shading)
	/*float lum = (color.r+color.g+color.b)/3.0;
	
	//On normalise la couleur
	color /= lum;
	
	//On seuille la luminance (cell shading)
	if(lum > 0.7)
		lum = 0.85;
	else
		if(lum > 0.6)
			lum = 0.65;
		else
			if(lum > 0.5)
				lum = 0.55;
			else
				if(lum > 0.4)
					lum = 0.45;	
				else
					if(lum > 0.3)
						lum = 0.35;	
					else
						lum = 0.3;
		
	//On ajoute un petit effet, plus c profond plus ca perd 30% de sa luminance, sauf si c'est le fond (sinon tout noir)		
	if(depth < 1)						
		lum *= (1-depth)*0.3 + 0.7;						
		
	//On applique le seuillage
	color *= lum;*/
	

	//Pour les contours		
	float x,y;
	float sommedepth=0;
	float nb = 0;

	for(x=-1;x<1;x++)                               
	{
		for(y=-1;y<1;y++)
		{
			vec2 coord =  gl_TexCoord[0].xy;
			coord.x += x*xstep;
			coord.y += y*ystep;
			float depthLocal = texture2D( Texture1 , coord ).r;	
			depthLocal = LinearizeDepth(depthLocal);
			sommedepth += abs(depth - depthLocal);
			nb++;
		}
	}
	
	sommedepth /= nb;
	
	
	/*
	
	Liant
	
	*/
	
	/*float coeff = depth;
	vec4 colorSum;
	
	if(coeff == 1.0)
	{
  	
  	nb = 0;
  	for(x=-25;x<25;x++)
  	{
  		  vec2 coord =  gl_TexCoord[0].xy;
    		coord.x += x*xstep;
    		 float dep = 1.0 - LinearizeDepth(texture2D( Texture1 , coord ).r);
    		colorSum += dep * texture2D( Texture0 , coord );	
    		nb += dep;
  	}
  	
  	for(y=-25;y<25;y++)
  	{
  		vec2 coord =  gl_TexCoord[0].xy;
  		coord.y += y*ystep;
  		float dep = 1.0 - LinearizeDepth(texture2D( Texture1 , coord ).r);
  		colorSum += dep * texture2D( Texture0 , coord );	
  		nb += dep;
  	}
  	
  	if(nb > 0)
  	 colorSum /= nb;
  }
	
	color = color * (1.0-coeff) + colorSum * coeff;   */
	
	

	//Flou 
	/*float coeff = 1.0-depth;
	vec4 colorSum;
	
	if(coeff > 0.0)
	{
  	
  	nb = 0;
  	for(x=-25;x<25;x++)
  	{
  		  vec2 coord =  gl_TexCoord[0].xy;
    		coord.x += x*xstep;
    		float dep = 1.0 - LinearizeDepth(texture2D( Texture1 , coord ).r);
    		colorSum += dep * texture2D( Texture0 , coord );	
    		nb += dep;
  	}
  	
  	for(y=-25;y<25;y++)
  	{
  		vec2 coord =  gl_TexCoord[0].xy;
  		coord.y += y*ystep;
  		float dep = 1.0 - LinearizeDepth(texture2D( Texture1 , coord ).r);
  		colorSum += dep * texture2D( Texture0 , coord );	
  		nb += dep;
  	}
  	
  	if(nb > 0)
  	 colorSum /= nb;
  }
	
	color = color * (1.0-coeff) + colorSum * coeff;   */

	//On trace noir si c'est le contour, mais de plus en plus noir (pas de disparition brutale du contour)
	/*if(sommedepth > 0.004)
		sommedepth = 0.004;
	if(sommedepth > 0.002)
	{
		sommedepth -= 0.002;
		sommedepth /= 0.004 - 0.002;
		color *= 0.20 * sommedepth;
	}*/

	if(sommedepth > 0.001)
	{
		color *= 0.5;
		//color = vec4(0,0,0,1);
	} 
		

	//Vignettage	
	vec2 vign = vec2( gl_TexCoord[0] );
	vign -= vec2(0.5,0.5);
	
	
	float facVign = max(0,(vign.x*vign.x + vign.y*vign.y)-0.35)*1.5;

	//color = vec4(facVign,0,0,1);

	color *= (1-facVign) ;

		
	color.a = 1.0;
	gl_FragColor = color;
}