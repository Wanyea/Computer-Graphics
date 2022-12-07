#ifndef TEXTURE_H
#define TEXTURE_H

#include "TexDB.H"
#include "Color.H"

#include "glwrap.H"


// IMPORTANT NOTE:  Must call contextInit() from within a GL context
// before you can use the texture!!



// As with OpenGL:
// 1 channel texture is luminance (grayscale).
// 2 channel texture is luminance and alpha.
// 3 channel texture is RGB.
// 4 channel texture is RGBA.

// values for texType...
enum {
  TEX_LUMINANCE = 1,
  TEX_LUMINANCE_ALPHA = 2,
  TEX_RGB = 3,
  TEX_RGBA = 4
};

class Texture
{
 public:

  Texture(str_ptr name, int texType, int width, int height);
  // this constructor loads the texture from a file
  Texture(str_ptr filename, str_ptr name,bool hasRealAlpha=false,int isStencil=false);
  
  virtual ~Texture();


  float* pixelPtr(int w, int h) {
    return &_texels[_texType*(w + h*_width)];
  }

  void setPixel(int w, int h, Color c) {
    float *a = c.array();
    for (int i=0;i<_texType;i++) {
      pixelPtr(w,h)[i] = a[i];
    }
  }

  Color getPixel(int w, int h) {
    float c[4];
    int i;
    c[0] = 0.0;
    c[1] = 0.0;
    c[2] = 0.0;
    c[3] = 1.0;
    for (i=0;i<_texType;i++)
      c[i] = pixelPtr(w,h)[i];
    Color col(c[0],c[1],c[2],c[3]);
    return col;
  }

  // expand to a 4 channel texture
  void expandToRGBA();
  // replace all occurances of col1 with col2
  void replaceCol(Color col1, Color col2);

  // initialize texture data with GL
  void contextInit();
  // after texture data has changed, pass changes on to GL
  void bindChangesToGL();


  str_ptr name()         const  { return _name; }
  str_ptr filename()     const  { return _filename; }
  int     type()         const  { return _texType; }
  int     width()        const  { return _width; }
  int     height()       const  { return _height; }
  GLuint  glName()              { return *_glname; }
  GLuint  glAlphaName()         { return *_glAlphaName; } 
  int     hasRealAlpha() const  { return _hasRealAlpha; }
  int     isStencil()    const  { return _isStencil;}
  double  aspect()       const  { 
    if (_height != 0)
      return (double)_width/(double)_height;
    else
      return 1.0;
  }

  void setHasRealAlpha(const int a) { _hasRealAlpha = a; }
  void setStencil(const int sten) {_isStencil=sten;}

  // fill the entire texture with this color
  void fill(Color c) {
    float *a = c.array();
    for (int h=0;h<_height;h++) {
      for (int w=0;w<_width;w++) {
    for (int i=0;i<_texType;i++) {
      pixelPtr(w,h)[i] = a[i];
    }
      }
    }
  }

  void print() {
    cout << "Texture '" << _name << "':" << endl;
    cout << "  Height = " << _height << "  Width = " << _width 
     << "  Num channels = "  << _texType << endl;
    for (int h=0;h<_height;h++) {
      for (int w=0;w<_width;w++) {
    cout << "("<<w<<","<<h<<") ";
    cout << getPixel(w,h) << endl;
      }
      cout << endl;
    }
  }

  void printRow(int h) {
    for (int w=0;w<_width;w++) {
      cout << "("<<w<<","<<h<<") ";
      cout << getPixel(w,h) << endl;
    }
  }

  int load(str_ptr filename); 
  int save(str_ptr filename);

  int loadRGB(const char *fname);
  int loadPNM(const char *fname);

  int loadTarga(const char *fname);
  int saveTarga(const char *fname);


 protected:

  str_ptr _name;
  str_ptr _filename;

  // stored in floats b/c GL converts everything to floats anyway??
  // check on that!
  GLfloat *_texels;
  isGlContextData<GLuint> _glname;
  isGlContextData<GLuint> _glAlphaName;
  

  int _texType;
  int _width;
  int _height;
  int _hasRealAlpha;
  int _isStencil;
  double _aspect;
};

#endif