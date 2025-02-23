#ifndef _SCRIPT_DRAWING_SURFACE_H_HEADER
#define _SCRIPT_DRAWING_SURFACE_H_HEADER

#include "dynobj/ags_cc_dynamic_object.h" 
#include "allegro.h"

struct ScriptDrawingSurface : AGSCCDynamicObject {
  int roomBackgroundNumber;
  int dynamicSpriteNumber;
  int dynamicSurfaceNumber;
  bool isLinkedBitmapOnly;
  BITMAP *linkedBitmapOnly;
  int currentColour;
  int currentColourScript;
  int highResCoordinates;
  int modified;
  int hasAlphaChannel;
  BITMAP* abufBackup;

  virtual int Dispose(const char *address, bool force);
  virtual const char *GetType();
  virtual int Serialize(const char *address, char *buffer, int bufsize);
  virtual void Unserialize(int index, const char *serializedData, int dataSize);
  BITMAP* GetBitmapSurface();
  void StartDrawing();
  void MultiplyThickness(int *adjustValue);
  void UnMultiplyThickness(int *adjustValue);
  void MultiplyCoordinates(int *xcoord, int *ycoord);
  void FinishedDrawing();
  void FinishedDrawingReadOnly();

  ScriptDrawingSurface();
};

#endif
