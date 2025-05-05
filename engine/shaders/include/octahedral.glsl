vec2 octahedral_mapping(vec3 co)
{
    // projection onto octahedron
	co /= dot( vec3(1), abs(co) );

    // out-folding of the downward faces
    if ( co.y < 0.0 ) {
		co.xy = (1.0 - abs(co.zx)) * sign(co.xz);
    }

	// mapping to [0;1]ˆ2 texture space
	return co.xy * 0.5 + 0.5;
}


vec3 octahedral_unmapping(vec2 co)
{
    co = co * 2.0 - 1.0;

    vec2 abs_co = abs(co);
    vec3 v = vec3(co, 1.0 - (abs_co.x + abs_co.y));

    if ( abs_co.x + abs_co.y > 1.0 ) {
        v.xy = (abs(co.yx) - 1.0) * -sign(co.xy);
    }

    return v;
}



/** Two-element sort: maybe swaps a and b so that a' = min(a, b), b' = max(a, b). */
void minSwap(inout float a, inout float b) {
    float temp = min(a, b);
    b = max(a, b);
    a = temp;
}


/** Sort the three values in v from least to 
    greatest using an exchange network (i.e., no branches) */
void sort(inout float3 v) {
    minSwap(v[0], v[1]);
    minSwap(v[1], v[2]);
    minSwap(v[0], v[1]);
}


/** Segments a ray into the piecewise-continuous rays or line segments that each lie within
    one Euclidean octant, which correspond to piecewise-linear projections in octahedral space.
        
    \param boundaryT  all boundary distance ("time") values in units of world-space distance 
      along the ray. In the (common) case where not all five elements are needed, the unused 
      values are all equal to tMax, creating degenerate ray segments.

    \param origin Ray origin in the Euclidean object space of the probe

    \param directionFrac 1 / ray.direction
 */
void computeRaySegments
   (in Point3           origin, 
    in vec3          directionFrac, 
    in float            tMin,
    in float            tMax,
    out float           boundaryTs[5]) {

    boundaryTs[0] = tMin;
    
    // Time values for intersection with x = 0, y = 0, and z = 0 planes, sorted
    // in increasing order
    vec3 t = origin * -directionFrac;
    sort(t);

    // Copy the values into the interval boundaries.
    // This loop expands at compile time and eliminates the
    // relative indexing, so it is just three conditional move operations
    for (int i = 0; i < 3; ++i) {
        boundaryTs[i + 1] = clamp(t[i], tMin, tMax);
    }

    boundaryTs[4] = tMax;
}
