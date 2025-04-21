//
//  ellipse.h
//  ZorSecure
//
//  Created by Joseph Marino on 4/9/13.
//  Copyright (c) 2013 Zortag. All rights reserved.
//

#ifndef ZorSecure_ellipse_h
#define ZorSecure_ellipse_h

#include <ctime>


/* Code to find the equation of a conic */
/*               Tom Davis              */
/*             April 12, 1996           */

/*
 * http://mathforum.org/library/drmath/view/51735.html
 */

/* toconic takes five points in homogeneous coordinates, and returns the
 * coefficients of a general conic equation in a, b, c, ..., f:
 *
 * a*x*x + b*x*y + c*y*y + d*x + e*y + f = 0.
 *
 * The routine returns 1 on success; 0 otherwise.  (It can fail, for
 * example, if there are duplicate points.
 *
 * Typically, the points will be finite, in which case the third (w)
 * coordinate for all the input vectors will be 1, although the code
 * deals cleanly with points at infinity.
 *
 * For example, to find the equation of the conic passing through (5, 0),
 * (-5, 0), (3, 2), (3, -2), and (-3, 2), set:
 *
 * p0[0] =  5, p0[1] =  0, p0[2] = 1,
 * p1[0] = -5, p1[1] =  0, p1[2] = 1,
 * p2[0] =  3, p2[1] =  2, p2[2] = 1,
 * p3[0] =  3, p3[1] = -2, p3[2] = 1,
 * p4[0] = -3, p4[1] =  2, p4[2] = 1.
 *
 * But if you want the equation of the hyperbola that is tangent to the
 * line 2x=y at infinity,  simply make one of the points be the point at
 * infinity along that line, for example:
 *
 * p0[0] = 1, p0[1] = 2, p0[2] = 0.
 */

static bool toconic(zor::Vector3f p[5], float conic[6])
{
    zor::Vector3f p0 = p[0], p1 = p[1], p2 = p[2], p3 = p[3], p4 = p[4];
    
    zor::Vector3f L0, L1, L2, L3;
    float A, B, C;
    zor::Vector3f Q;
    float a1, a2, b1, b2, c1, c2;
    float x0, x4, y0, y4, w0, w4;
    float aa, bb, cc, dd, ee, ff;
    float y4w0, w4y0, w4w0, y4y0, x4w0, w4x0, x4x0, y4x0, x4y0;
    float a1a2, a1b2, a1c2, b1a2, b1b2, b1c2, c1a2, c1b2, c1c2;
    
    //L0 = p0.cross(p1);
    //L1 = p1.cross(p2);
    //L2 = p2.cross(p3);
    //L3 = p3.cross(p4);
    //Q = L0.cross(L3);
	L0 = zor::cross_product(p0, p1);
    L1 = zor::cross_product(p1, p2);
    L2 = zor::cross_product(p2, p3);
    L3 = zor::cross_product(p3, p4);
    Q = zor::cross_product(L0, L3);
    A = Q.x(); B = Q.y(); C = Q.z();
    a1 = L1.x(); b1 = L1.y(); c1 = L1.z();
    a2 = L2.x(); b2 = L2.y(); c2 = L2.z();
    x0 = p0.x(); y0 = p0.y(); w0 = p0.z();
    x4 = p4.x(); y4 = p4.y(); w4 = p4.z();
    
    y4w0 = y4*w0;
    w4y0 = w4*y0;
    w4w0 = w4*w0;
    y4y0 = y4*y0;
    x4w0 = x4*w0;
    w4x0 = w4*x0;
    x4x0 = x4*x0;
    y4x0 = y4*x0;
    x4y0 = x4*y0;
    a1a2 = a1*a2;
    a1b2 = a1*b2;
    a1c2 = a1*c2;
    b1a2 = b1*a2;
    b1b2 = b1*b2;
    b1c2 = b1*c2;
    c1a2 = c1*a2;
    c1b2 = c1*b2;
    c1c2 = c1*c2;
    
    aa = -A*a1a2*y4w0
    +A*a1a2*w4y0
    -B*b1a2*y4w0
    -B*c1a2*w4w0
    +B*a1b2*w4y0
    +B*a1c2*w4w0
    +C*b1a2*y4y0
    +C*c1a2*w4y0
    -C*a1b2*y4y0
    -C*a1c2*y4w0;
    
    cc =  A*c1b2*w4w0
    +A*a1b2*x4w0
    -A*b1c2*w4w0
    -A*b1a2*w4x0
    +B*b1b2*x4w0
    -B*b1b2*w4x0
    +C*b1c2*x4w0
    +C*b1a2*x4x0
    -C*c1b2*w4x0
    -C*a1b2*x4x0;
    
    ff =  A*c1a2*y4x0
    +A*c1b2*y4y0
    -A*a1c2*x4y0
    -A*b1c2*y4y0
    -B*c1a2*x4x0
    -B*c1b2*x4y0
    +B*a1c2*x4x0
    +B*b1c2*y4x0
    -C*c1c2*x4y0
    +C*c1c2*y4x0;
    
    bb =  A*c1a2*w4w0
    +A*a1a2*x4w0
    -A*a1b2*y4w0
    -A*a1c2*w4w0
    -A*a1a2*w4x0
    +A*b1a2*w4y0
    +B*b1a2*x4w0
    -B*b1b2*y4w0
    -B*c1b2*w4w0
    -B*a1b2*w4x0
    +B*b1b2*w4y0
    +B*b1c2*w4w0
    -C*b1c2*y4w0
    -C*b1a2*x4y0
    -C*b1a2*y4x0
    -C*c1a2*w4x0
    +C*c1b2*w4y0
    +C*a1b2*x4y0
    +C*a1b2*y4x0
    +C*a1c2*x4w0;
    
    dd = -A*c1a2*y4w0
    +A*a1a2*y4x0
    +A*a1b2*y4y0
    +A*a1c2*w4y0
    -A*a1a2*x4y0
    -A*b1a2*y4y0
    +B*b1a2*y4x0
    +B*c1a2*w4x0
    +B*c1a2*x4w0
    +B*c1b2*w4y0
    -B*a1b2*x4y0
    -B*a1c2*w4x0
    -B*a1c2*x4w0
    -B*b1c2*y4w0
    +C*b1c2*y4y0
    +C*c1c2*w4y0
    -C*c1a2*x4y0
    -C*c1b2*y4y0
    -C*c1c2*y4w0
    +C*a1c2*y4x0;
    
    ee = -A*c1a2*w4x0
    -A*c1b2*y4w0
    -A*c1b2*w4y0
    -A*a1b2*x4y0
    +A*a1c2*x4w0
    +A*b1c2*y4w0
    +A*b1c2*w4y0
    +A*b1a2*y4x0
    -B*b1a2*x4x0
    -B*b1b2*x4y0
    +B*c1b2*x4w0
    +B*a1b2*x4x0
    +B*b1b2*y4x0
    -B*b1c2*w4x0
    -C*b1c2*x4y0
    +C*c1c2*x4w0
    +C*c1a2*x4x0
    +C*c1b2*y4x0
    -C*c1c2*w4x0
    -C*a1c2*x4x0;
    
    if (aa != 0.0) {
        bb /= aa; cc /= aa; dd /= aa; ee /= aa; ff /= aa; aa = 1.f;
    } else if (bb != 0.0) {
        cc /= bb; dd /= bb; ee /= bb; ff /= bb; bb = 1.f;
    } else if (cc != 0.0) {
        dd /= cc; ee /= cc; ff /= cc; cc = 1.f;
    } else if (dd != 0.0) {
        ee /= dd; ff /= dd; dd = 1.f;
    } else if (ee != 0.0) {
        ff /= ee; ee = 1.f;
    } else {
        return false;
    }
    
    conic[0] = aa;
    conic[1] = bb;
    conic[2] = cc;
    conic[3] = dd;
    conic[4] = ee;
    conic[5] = ff;
    return true;
}


struct Parameters {
    float A, B, C, D, E, F;
    int count;
    Parameters() { A = 0; B = 0; C = 0; D = 0; E = 0; F = 0; count = 0; }
};

static void randomHoughEllipseConicEquation(const zor::GrayImage8u &edge, float equation[6])//, const int num_matches, const int num_tries)
{
    int count = 0;
    for (int i = 0; i < edge.num_pixels(); ++i)
        if (edge.pixel(i) == 255)
            count++;
    
    std::vector<zor::Vector2i> points;
    points.reserve(count);
    for (int i = 0; i < edge.num_pixels(); ++i)
        if (edge.pixel(i) == 255)
            points.push_back(zor::Vector2i(i%edge.width(), i/edge.width()));
    
    zor::Vector3f p[5];
    float A, B, C, D, E, F;
    
    std::vector<Parameters> params;
    
    int theCount = 0;
    int tries = 0;
    if (points.size() == 0)
        tries = 10000;
    float conic[6];
    std::srand((unsigned int)time(NULL));
    
    //for (int i = 0; i < edge.num_pixels(); ++i)
    //    edge.pixel(i) = 0;
    
    while (theCount < 64 && tries++ < 10000) // was 32 & 10000
	//while (theCount < num_matches && tries++ < num_tries)
    {
        for (int j = 0; j < 5; ++j) {
            int r = std::rand() % count;
            p[j].x() = (float)points[r].x();
            p[j].y() = (float)points[r].y();
            p[j].z() = 1.f;
            //if (edge.pixel(p[j].x(), p[j].y()) < 255)
            //    edge.pixel(p[j].x(), p[j].y()) += 10;
        }
        
        if (!toconic(p, conic)) // error in fitting ellipse
            continue;
        if (conic[1]*conic[1] - 4*conic[0]*conic[2] >= 0) // not an ellipse
            continue;
        if (conic[2] < 0.8 || conic[2] > 1.2) // too squished
            continue;
        if (conic[1] < -0.2 || conic[1] > 0.2) // too angled
            continue;
        
        A = conic[0]; B = conic[1]; C = conic[2]; D = conic[3]; E = conic[4]; F = conic[5];
        int index = -1;
        for (int j = 0; j < (int)params.size(); ++j) {
            //float diffA = std::abs(A - params[j].A);
            //float diffB = std::abs(B - params[j].B);
            float diffC = std::abs(C - params[j].C);
            float diffD = std::abs(D - params[j].D);
            float diffE = std::abs(E - params[j].E);
            //float diffF = std::abs(F - params[j].F);
            if (diffC < 0.05 && diffD < 4 && diffE < 4) {
                if (index == -1)
                    index = j;
                else {
                    // check against previous close match
                }
            }
        }
        
        if (index == -1) {
            params.push_back(Parameters());
            params[params.size()-1].A = A;
            params[params.size()-1].B = B;
            params[params.size()-1].C = C;
            params[params.size()-1].D = D;
            params[params.size()-1].E = E;
            params[params.size()-1].F = F;
            params[params.size()-1].count = 1;
            theCount = 1;
        }
        else {
            params[index].A = (params[index].A * params[index].count + A) / (params[index].count + 1);
            params[index].B = (params[index].B * params[index].count + B) / (params[index].count + 1);
            params[index].C = (params[index].C * params[index].count + C) / (params[index].count + 1);
            params[index].D = (params[index].D * params[index].count + D) / (params[index].count + 1);
            params[index].E = (params[index].E * params[index].count + E) / (params[index].count + 1);
            params[index].F = (params[index].F * params[index].count + F) / (params[index].count + 1);
            params[index].count += 1;
            theCount = params[index].count;
        }
    }
    
    //NSLog(@"theCount: %d  tries: %d", theCount, tries);
    
    if (params.size() == 0) // something went wrong!
        return;
    
    int maxCount = params[0].count;
    int index = 0;
    for (int i = 1; i < (int)params.size(); ++i) {
        if (params[i].count > maxCount) {
            maxCount = params[i].count;
            index = i;
        }
    }
    
    equation[0] = params[index].A;
    equation[1] = params[index].B;
    equation[2] = params[index].C;
    equation[3] = params[index].D;
    equation[4] = params[index].E;
    equation[5] = params[index].F;
}


static void ellipse_conic_to_parametric(float conic_equation[6], float parametric_equation[5])
{
    float A = conic_equation[0];
    float B = conic_equation[1];
    float C = conic_equation[2];
    float D = conic_equation[3];
    float E = conic_equation[4];
    float F = conic_equation[5];
    
	/*
    zor::Matrix3x3f M0;
    M0[0] = F;      M0[1] = D/2;    M0[2] = E/2;
    M0[3] = D/2;    M0[4] = A;      M0[5] = B/2;
    M0[6] = E/2;    M0[7] = B/2;    M0[8] = C;
    
    zor::Matrix2x2f M;
    M[0] = A;   M[1] = B/2;
    M[2] = B/2; M[3] = C;
	*/

	zor::Matrix3x3f M0;
    M0(0,0) = F;      M0(0,1) = D/2;    M0(0,2) = E/2;
    M0(1,0) = D/2;    M0(1,1) = A;      M0(1,2) = B/2;
    M0(2,0) = E/2;    M0(2,1) = B/2;    M0(2,2) = C;
    
    zor::Matrix2x2f M;
    M(0,0) = A;   M(0,1) = B/2;
    M(1,0) = B/2; M(1,1) = C;
    
    //float detM0 = M0.determinant();
    //float detM = M.determinant();
	float detM0 = zor::determinant(M0);
	float detM = zor::determinant(M);
    
    float eigen[2];
    //M.eigenvalues(eigen);
	zor::eigenvalues(M, eigen);
    float lambda1 = eigen[0], lambda2 = eigen[1];
    if (std::abs(lambda1 - A) > std::abs(lambda1 - C)) {
        float temp = lambda1;
        lambda1 = lambda2;
        lambda2 = temp;
    }
    
    //NSLog(@"detM0=%f detM=%f lambda1=%f lambda2=%f", detM0, detM, lambda1, lambda2);
    
    //float a = std::sqrt( -detM0 / (detM*lambda1) );
    //float b = std::sqrt( -detM0 / (detM*lambda2) );
    // dirty fix? (sometimes detM0 is not negative?)
    float a = -detM0 / (detM*lambda1);
    float b = -detM0 / (detM*lambda2);
    if (a < 0 || b < 0) printf("Flipping detM0");
    if (a < 0) a *= -1;
    if (b < 0) b *= -1;
    a = std::sqrt(a);
    b = std::sqrt(b);
    float h = (B*E - 2*C*D) / (4*A*C - B*B);
    float k = (B*D - 2*A*E) / (4*A*C - B*B);
    float tau = std::atan(B / (A - C)) / 2;
    
    parametric_equation[0] = a;
    parametric_equation[1] = b;
    parametric_equation[2] = h;
    parametric_equation[3] = k;
    parametric_equation[4] = tau;
}


#endif
