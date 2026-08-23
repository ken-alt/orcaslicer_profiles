// ---------------------------------------------------------------------------
//  Regression tests for Display_Space_IDT.dctl
//
//  Compiles the DCTL as plain C++ against a shim that emulates the DCTL
//  runtime (float3, the _xxx math builtins, DEFINE_UI_PARAMS), so the
//  transform can be exercised without launching Resolve. Catches syntax
//  errors, NaN/Inf, non-monotonic tone curves, value blow-ups on sub-black
//  and super-white input, and overlay pixels leaking outside their panel.
//
//  Run:  ./run_tests.sh
// ---------------------------------------------------------------------------
#include <cstdio>
#include <cmath>
#include <cstdlib>
#define __DEVICE__ static inline
#define __CONSTANT__ static const
#define DEFINE_UI_PARAMS(...)
struct float3 { float x, y, z; };
static inline float3 make_float3(float a,float b,float c){float3 v{a,b,c};return v;}
static inline float _fabs(float a){return fabsf(a);} static inline float _powf(float a,float b){return powf(a,b);}
static inline float _log2f(float a){return log2f(a);} static inline float _exp2f(float a){return exp2f(a);}
static inline float _sqrtf(float a){return sqrtf(a);} static inline float _fmaxf(float a,float b){return fmaxf(a,b);}
static inline float _fminf(float a,float b){return fminf(a,b);} static inline float _copysignf(float a,float b){return copysignf(a,b);}
static inline float _clampf(float v,float a,float b){return fminf(fmaxf(v,a),b);}
static inline float _mix(float a,float b,float t){return a+(b-a)*t;} static inline float _floor(float a){return floorf(a);}
enum { DSP_709_24, DSP_709_22, DSP_709_SRGB, DSP_709_709, DSP_P3_26, DSP_P3_24, DSP_2020_24, DSP_2020_PQ };
enum { DST_DWG_LIN, DST_DWG_DI, DST_AP1_CCT, DST_AP0_LIN, DST_709_LIN };
static float p_Unroll=0.5f,p_Toe=0.5f,p_Expand=0.5f,p_PHC=0.5f,p_Black=0.0f,p_Contrast=1.0f;
static int p_ShowCurve=0,p_Display=DSP_709_24,p_Dest=DST_DWG_LIN;
#include "../Display_Space_IDT.dctl"

int main(){
  int fails=0;

  printf("== A. neutral monotonicity over full param grid (incl. sub-black input) ==\n");
  int nonmono=0,nonfin=0;
  for(int u=0;u<=4;u++)for(int t=0;t<=4;t++)for(int e=0;e<=4;e++)for(int h=0;h<=2;h++)
  for(int bk=0;bk<=2;bk++)for(int c=0;c<=2;c++)for(int d=0;d<8;d++){
    p_Unroll=u/4.0f;p_Toe=t/4.0f;p_Expand=e/4.0f;p_PHC=h/2.0f;
    p_Black=-0.05f+0.075f*bk;p_Contrast=0.5f+0.75f*c;p_Display=d;
    float prev=-1e30f;
    for(int i=-20;i<=1020;i++){
      float v=i/1000.0f; float3 o=transform(1920,1080,900,500,v,v,v);
      if(!std::isfinite(o.x)||!std::isfinite(o.y)||!std::isfinite(o.z)){nonfin++;break;}
      if(o.x<prev-1e-5f*fmaxf(1.0f,fabsf(prev))){nonmono++;break;} prev=o.x;
    }
  }
  printf("   non-finite: %d   non-monotonic: %d   %s\n",nonfin,nonmono,(nonfin||nonmono)?"FAIL":"PASS");
  fails += (nonfin||nonmono);

  printf("\n== B. no blow-ups: random colours incl. sub-black / super-white ==\n");
  printf("   (dark-in/bright-out checked on linear destinations; DI and ACEScct\n    legitimately scale negatives by their linear-segment slope)\n");
  srand(7); float worstmag=0,worstin=0; float wr=0,wg=0,wb=0,wo=0;
  int bad=0;
  for(int n=0;n<400000;n++){
    p_Unroll=(rand()%1001)/1000.0f;p_Toe=(rand()%1001)/1000.0f;p_Expand=(rand()%1001)/1000.0f;
    p_PHC=(rand()%1001)/1000.0f;p_Black=-0.05f+0.15f*(rand()%1001)/1000.0f;
    p_Contrast=0.5f+1.5f*(rand()%1001)/1000.0f;p_Display=rand()%8;p_Dest=rand()%5;
    int lineardest = (p_Dest==DST_DWG_LIN||p_Dest==DST_AP0_LIN||p_Dest==DST_709_LIN);
    float r=-0.1f+1.2f*(rand()%1001)/1000.0f,g=-0.1f+1.2f*(rand()%1001)/1000.0f,b=-0.1f+1.2f*(rand()%1001)/1000.0f;
    float3 o=transform(1920,1080,900,500,r,g,b);
    if(!std::isfinite(o.x)||!std::isfinite(o.y)||!std::isfinite(o.z)){ bad++; continue; }
    float mag=fmaxf(fmaxf(fabsf(o.x),fabsf(o.y)),fabsf(o.z));
    float in =fmaxf(fmaxf(fabsf(r),fabsf(g)),fabsf(b));
    if(mag>worstmag){worstmag=mag;worstin=in;wr=r;wg=g;wb=b;wo=o.x;}
    // a dark input must never produce a bright output
    if(lineardest && in<0.2f && mag>1.0f){ if(bad<5) printf("   BLOWUP in[%.3f %.3f %.3f]->[%.3f %.3f %.3f]\n",r,g,b,o.x,o.y,o.z); bad++; }
  }
  printf("   non-finite/blowups: %d   worst |out|=%.3f (from in max %.3f, rgb %.2f %.2f %.2f, o.x %.3f)\n",
         bad,worstmag,worstin,wr,wg,wb,wo);
  printf("   %s\n", bad?"FAIL":"PASS");
  fails += (bad!=0);

  printf("\n== C. hue stability through the unroll (PHC off vs on), 709/2.4 -> DWG lin ==\n");
  p_Display=DSP_709_24;p_Dest=DST_DWG_LIN;p_Unroll=0.5f;p_Toe=0.5f;p_Expand=0.0f;p_Black=0.0f;p_Contrast=1.0f;
  float hot[3]={0.99f,0.86f,0.62f};   // warm blown highlight
  for(int i=0;i<3;i++){
    p_PHC=i/2.0f;
    float3 o=transform(1920,1080,900,500,hot[0],hot[1],hot[2]);
    float mx=fmaxf(fmaxf(o.x,o.y),o.z);
    printf("   PHC=%.1f -> [%7.4f %7.4f %7.4f]  normalised [%.3f %.3f %.3f]\n",
           p_PHC,o.x,o.y,o.z,o.x/mx,o.y/mx,o.z/mx);
  }
  float3 src=transform(1920,1080,900,500,hot[0]*0.4f,hot[1]*0.4f,hot[2]*0.4f);
  float sm=fmaxf(fmaxf(src.x,src.y),src.z);
  printf("   same hue at 40%% exposure (reference ratios): [%.3f %.3f %.3f]\n",src.x/sm,src.y/sm,src.z/sm);

  printf("\n== D. bypass check: all sliders at identity must be a pure colour-space convert ==\n");
  p_Unroll=0;p_Toe=0;p_Expand=0;p_PHC=0;p_Black=0;p_Contrast=1;p_Dest=DST_709_LIN;p_Display=DSP_709_24;
  float maxerr=0;
  for(int i=0;i<=100;i++){ float v=i/100.0f;
    float3 o=transform(1920,1080,900,500,v,v*0.7f,v*0.4f);
    maxerr=fmaxf(maxerr,fabsf(o.x-powf(v,2.4f)));
  }
  printf("   max |out - v^2.4| = %.3e   %s\n",maxerr,maxerr<1e-5?"PASS":"FAIL");
  fails += (maxerr>=1e-5f);

  printf("\n== E. overlay does not leak outside its panel ==\n");
  p_ShowCurve=1;p_Dest=DST_DWG_LIN;p_Unroll=0.5f;p_Toe=0.5f;p_Expand=0.5f;p_PHC=0.5f;
  int leak=0,inside=0;
  for(int Y=0;Y<1080;Y+=1) for(int X=0;X<1920;X+=1){
    float3 o=transform(1920,1080,X,Y,0.5f,0.5f,0.5f);
    bool changed = fabsf(o.x-0.203712f)>1e-4f;
    float px=(float)X, py=1080.0f-(float)Y, side=0.30f*1920.0f, m=0.03f*1920.0f;
    bool inpanel = (px>=m&&px<=m+side&&py>=m&&py<=m+side);
    if(changed&&!inpanel) leak++;
    if(changed&&inpanel) inside++;
  }
  printf("   pixels changed outside panel: %d (expect 0)   inside: %d   %s\n",leak,inside,leak?"FAIL":"PASS");
  fails += (leak!=0);

  printf("\n%s\n", fails?"=== SOME CHECKS FAILED ===":"=== ALL CHECKS PASSED ===");
  return fails;
}
