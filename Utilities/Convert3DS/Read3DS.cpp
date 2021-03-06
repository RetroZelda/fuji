
/*----------------------------------------------------------------------------*\
     This is a lib which reads 3d-studio binary files from version 3.0
     and higher
     (v1.05)
     author: Martin van Velsen
             ( and some great help by Gert van der Spoel )
     email:  vvelsen@ronix.ptf.hro.nl
\*----------------------------------------------------------------------------*/
#ifndef __3DSBIN_C__
#define __3DSBIN_C__

#include "3DS.h"

/*----------------------------------------------------------------------------*/
unsigned char ReadChar (void)
{
 return (fgetc (bin3ds));
}
/*----------------------------------------------------------------------------*/
unsigned int ReadInt (void)
{
 return (ReadChar () | (ReadChar () << 8));
}
/*----------------------------------------------------------------------------*/
unsigned long ReadLong (void)
{
 unsigned long temp1,temp2;

 temp1=(ReadChar () | (ReadChar () << 8));
 temp2=(ReadChar () | (ReadChar () << 8));

 return (temp1+(temp2*0x10000L));
}
/*----------------------------------------------------------------------------*/
unsigned long ReadChunkPointer (void)
{
 return (ReadLong ());
}
/*----------------------------------------------------------------------------*/
unsigned long GetChunkPointer (void)
{
 return (ftell (bin3ds)-2); // compensate for the already read Marker
}
/*----------------------------------------------------------------------------*/
void ChangeChunkPointer (unsigned long temp_pointer)
{
 fseek (bin3ds,temp_pointer,SEEK_SET);
}
/*----------------------------------------------------------------------------*/
int ReadName (void)
{
 unsigned int teller=0;
 unsigned char letter;

 strcpy (temp_name,"Default name");

 letter=ReadChar ();
 if (letter==0) return (-1); // dummy object
 temp_name [teller]=letter;
 teller++;

 do
 {
  letter=ReadChar ();
  temp_name [teller]=letter;
  teller++;
 }
 while ((letter!=0) && (teller<12));

 temp_name [teller-1]=0;

 #ifdef __DEBUG__
  printf ("     Found name : %s\n",temp_name);
 #endif
 return (0);
}
/*----------------------------------------------------------------------------*/
int ReadLongName (void)
{
 unsigned int teller=0;
 unsigned char letter;

 strcpy (temp_name,"Default name");

 letter=ReadChar ();
 if (letter==0) return (-1); // dummy object
 temp_name [teller]=letter;
 teller++;

 do
 {
  letter=ReadChar ();
  temp_name [teller]=letter;
  teller++;
 }
 while (letter!=0);

 temp_name [teller-1]=0;

 #ifdef __DEBUG__
   printf ("Found name : %s\n",temp_name);
 #endif
 return (0);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadUnknownChunk (unsigned int chunk_id)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 chunk_id=chunk_id;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadRGBColor (void)
{
 float rgb_val [3];

 for (int i=0;i<3;i++)
  fread (&(rgb_val [i]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Color (RGB) def of: R:%5.2f,G:%5.2f,B:%5.2f\n",
          rgb_val [0],
          rgb_val [1],
          rgb_val [2]);
 #endif

 return (12L);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadTrueColor (void)
{
 unsigned char true_c_val [3];

 for (int i=0;i<3;i++)
  true_c_val [i]=ReadChar ();

 #ifdef __DEBUG__
 printf ("     Found Color (24bit) def of: R:%d,G:%d,B:%d\n",
          true_c_val [0],
          true_c_val [1],
          true_c_val [2]);
 #endif

 return (3L);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadBooleanChunk (unsigned char *boolean)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 *boolean=ReadChar ();

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadSpotChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float target [4];
 float hotspot,falloff;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(target [0]),sizeof (float),1,bin3ds);
 fread (&(target [1]),sizeof (float),1,bin3ds);
 fread (&(target [2]),sizeof (float),1,bin3ds);
 fread (&hotspot,sizeof (float),1,bin3ds);
 fread (&falloff,sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("      The target of the spot is at: X:%5.2f Y:%5.2f Y:%5.2f\n",
          target [0],
          target [1],
          target [2]);
 printf ("      The hotspot of this light is : %5.2f\n",hotspot);
 printf ("      The falloff of this light is : %5.2f\n",falloff);
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadLightChunk (void)
{
 unsigned char end_found=FALSE,boolean;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer
 float light_coors [3];

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(light_coors [0]),sizeof (float),1,bin3ds);
 fread (&(light_coors [1]),sizeof (float),1,bin3ds);
 fread (&(light_coors [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found light at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          light_coors [0],
          light_coors [1],
          light_coors [2]);
 #endif

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case LIT_UNKNWN01 :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Light unknown chunk id of %0X\n",LIT_UNKNWN01);
                           #endif
                           tellertje+=ReadUnknownChunk (LIT_UNKNWN01);
                           break;
        case LIT_OFF      :
                           #ifdef __DEBUG__
                           printf (">>>>> Light is (on/off) chunk: %0X\n",LIT_OFF);
                           #endif
                           tellertje+=ReadBooleanChunk (&boolean);
                           #ifdef __DEBUG__
                           if (boolean==TRUE)
                             printf ("      Light is on\n");
                           else
                             printf ("      Light is off\n");
                           #endif
                           break;
        case LIT_SPOT     :
                           #ifdef __DEBUG__
                           printf (">>>>> Light is SpotLight: %0X\n",TRI_VERTEXL);
                           #endif
                           tellertje+=ReadSpotChunk ();
                           break;
        case COL_RGB      :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Color def (RGB) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadRGBColor ();
                           break;
        case COL_TRU      :
                           #ifdef __DEBUG__
                           printf (">>>>> Found Color def (24bit) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadTrueColor ();
                           break;
        default           :break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadCameraChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float camera_eye [3];
 float camera_focus [3];
 float rotation,lens;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 fread (&(camera_eye [0]),sizeof (float),1,bin3ds);
 fread (&(camera_eye [1]),sizeof (float),1,bin3ds);
 fread (&(camera_eye [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Camera viewpoint at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          camera_eye [0],
          camera_eye [1],
          camera_eye [2]);
 #endif

 fread (&(camera_focus [0]),sizeof (float),1,bin3ds);
 fread (&(camera_focus [1]),sizeof (float),1,bin3ds);
 fread (&(camera_focus [2]),sizeof (float),1,bin3ds);

 #ifdef __DEBUG__
 printf ("     Found Camera focus coors at coordinates: X: %5.2f, Y: %5.2f,Z: %5.2f\n",
          camera_focus [0],
          camera_focus [1],
          camera_focus [2]);
 #endif

 fread (&rotation,sizeof (float),1,bin3ds);
 fread (&lens,sizeof (float),1,bin3ds);
 #ifdef __DEBUG__
 printf ("     Rotation of camera is:  %5.4f\n",rotation);
 printf ("     Lens in used camera is: %5.4fmm\n",lens);
 #endif

 if ((temp_pointer-38)>0) // this means more chunks are to follow
 {
  #ifdef __DEBUG__
  printf ("     **** found extra cam chunks ****\n");
  #endif
  if (ReadInt ()==CAM_UNKNWN01)
  {
   #ifdef __DEBUG__
   printf ("     **** Found cam 1 type ch ****\n");
   #endif
   ReadUnknownChunk (CAM_UNKNWN01);
  }
  if (ReadInt ()==CAM_UNKNWN02)
  {
   #ifdef __DEBUG__
   printf ("     **** Found cam 2 type ch ****\n");
   #endif
   ReadUnknownChunk (CAM_UNKNWN02);
  }
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadVerticesChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 float vertices [3]; // x,y,z
 unsigned int numb_v;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();
 numb_vertices  =ReadInt ();

 #ifdef __DEBUG__
 printf ("      Found (%d) number of vertices\n",numb_vertices);
 #endif

 for (int i=0;i<numb_vertices;i++)
 {
  fread (&(vertices [0]),sizeof (float),1,bin3ds);
  fread (&(vertices [1]),sizeof (float),1,bin3ds);
  fread (&(vertices [2]),sizeof (float),1,bin3ds);

  #ifdef __DEBUG__
  printf ("      Vertex nr%4d: X: %5.2f  Y: %5.2f  Z:%5.2f\n",
           i,
           vertices [0],
           vertices [1],
           vertices [2]);
  #endif
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/ unsigned long current_pointer;
unsigned long ReadSmoothingChunk ()
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long smoothing;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 for (int i=0;i<numb_faces;i++)
 {
  smoothing=ReadLong();
  smoothing=smoothing; // compiler warnig depressor *>:)
  #ifdef __DEBUG__
  printf ("      The smoothing group for face [%5d] is %d\n",i,smoothing);
  #endif
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadFacesChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned int temp_diff;
 unsigned int faces [6]; // a,b,c,Diff (Diff= AB: BC: CA: )

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();
 numb_faces     =ReadInt ();
 #ifdef __DEBUG__
 printf ("      Found (%d) number of faces\n",numb_faces);
 #endif

 for (int i=0;i<numb_faces;i++)
 {
  faces [0]=ReadInt ();
  faces [1]=ReadInt ();
  faces [2]=ReadInt ();
  temp_diff=ReadInt () & 0x000F;
  faces [3]=(temp_diff & 0x0004) >> 2;
  faces [4]=(temp_diff & 0x0002) >> 1;
  faces [5]=(temp_diff & 0x0001);

  #ifdef __DEBUG__
  printf ("      Face nr:%d, A: %d  B: %d  C:%d , AB:%d  BC:%d  CA:%d\n",
           i,
           faces [0],
           faces [1],
           faces [2],
           faces [3],
           faces [4],
           faces [5]);
  #endif
 }

 if (ReadInt ()==TRI_SMOOTH)
  ReadSmoothingChunk ();
 #ifdef __DEBUG__
 else
  printf ("      No smoothing groups found, assuming autosmooth\n");
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadTranslationChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 for (int j=0;j<4;j++)
 {
   for (int i=0;i<3;i++)
    fread (&(trans_mat [j][i]),sizeof (float),1,bin3ds);
 }

 trans_mat [0][3]=0;
 trans_mat [1][3]=0;
 trans_mat [2][3]=0;
 trans_mat [3][3]=1;

 #ifdef __DEBUG__
 printf ("     The translation matrix is:\n");
 for (int i=0;i<4;i++)
     printf ("      | %5.2f %5.2f %5.2f %5.2f |\n",
              trans_mat [i][0],
              trans_mat [i][1],
              trans_mat [i][2],
              trans_mat [i][3]);
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadObjChunk (void)
{
 unsigned char end_found=FALSE,boolean=TRUE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case TRI_VERTEXL :
                          #ifdef __DEBUG__
                          printf (">>>>> Found Object vertices chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadVerticesChunk ();
                          break;
        case TRI_FACEL1  :
                          #ifdef __DEBUG__
                          printf (">>>>> Found Object faces (1) chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadFacesChunk ();
                          break;
        case TRI_FACEL2  :
                          #ifdef __DEBUG__
                          printf (">>>>> Found Object faces (2) chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadUnknownChunk (temp_int);
                          break;
/*
		case TRI_TRANSL  :
                          #ifdef __DEBUG__
                          printf (">>>>> Found Object translation chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadTranslationChunk ();
                          break;
*/
        case TRI_VISIBLE :
                          #ifdef __DEBUG__
                          printf (">>>>> Found Object vis/invis chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadBooleanChunk (&boolean);

                          #ifdef __DEBUG__
                          if (boolean==TRUE)
                             printf ("      Object is (visible)\n");
                          else
                             printf ("      Object is (not visible)\n");
                          #endif
                          break;
        default:          break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadObjectChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 if (ReadName ()==-1)
 {
  #ifdef __DEBUG__
  printf (">>>>* Dummy Object found\n");
  #endif
 }

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case OBJ_UNKNWN01:tellertje+=ReadUnknownChunk (OBJ_UNKNWN01);break;
        case OBJ_UNKNWN02:tellertje+=ReadUnknownChunk (OBJ_UNKNWN02);break;
        case OBJ_TRIMESH :
                          #ifdef __DEBUG__
                          printf (">>>> Found Obj/Mesh chunk id of %0X\n",OBJ_TRIMESH);
                          #endif
                          tellertje+=ReadObjChunk ();
                          break;
        case OBJ_LIGHT   :
                          #ifdef __DEBUG__
                          printf (">>>> Found Light chunk id of %0X\n",OBJ_LIGHT);
                          #endif
                          tellertje+=ReadLightChunk ();
                          break;
        case OBJ_CAMERA  :
                          #ifdef __DEBUG__
                          printf (">>>> Found Camera chunk id of %0X\n",OBJ_CAMERA);
                          #endif
                          tellertje+=ReadCameraChunk ();
                          break;
        default:          break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadBackgrChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case COL_RGB :
                      #ifdef __DEBUG__
                      printf (">> Found Color def (RGB) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadRGBColor ();
                      break;
        case COL_TRU :
                      #ifdef __DEBUG__
                      printf (">> Found Color def (24bit) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadTrueColor ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadAmbientChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L; // 2 id + 4 pointer

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case COL_RGB :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (RGB) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadRGBColor ();
                      break;
        case COL_TRU :
                      #ifdef __DEBUG__
                      printf (">>>> Found Color def (24bit) chunk id of %0X\n",temp_int);
                      #endif
                      tellertje+=ReadTrueColor ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long FindCameraChunk (void)
{
 long temp_pointer=0L;

 for (int i=0;i<12;i++)
  ReadInt ();

 temp_pointer=11L;
 temp_pointer=ReadName ();

 #ifdef __DEBUG__
 if (temp_pointer==-1)
   printf (">>>>* No Camera name found\n");
 #endif

 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadViewPortChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned int port,attribs;

 views_read++;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 attribs=ReadInt ();
 if (attribs==3)
 {
  #ifdef __DEBUG__
  printf ("<Snap> active in viewport\n");
  #endif
 }
 if (attribs==5)
 {
  #ifdef __DEBUG__
  printf ("<Grid> active in viewport\n");
  #endif
 }

 for (int i=1;i<6;i++) ReadInt (); // read 5 ints to get to the viewport

 port=ReadInt ();
 if ((port==0xFFFF) || (port==0))
 {
   FindCameraChunk ();
   port=CAMERA;
 }

 #ifdef __DEBUG__
 printf ("Reading [%s] information with id:%d\n",viewports [port],port);
 #endif

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadViewChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case EDIT_VIEW_P1 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport1 chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadViewPortChunk ();
                           break;
        case EDIT_VIEW_P2 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport2 (bogus) chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadUnknownChunk (EDIT_VIEW_P2);
                           break;
       case EDIT_VIEW_P3 :
                           #ifdef __DEBUG__
                           printf (">>>> Found Viewport chunk id of %0X\n",temp_int);
                           #endif
                           tellertje+=ReadViewPortChunk ();
                           break;
        default           :break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;

   if (views_read>3)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadMatDefChunk (void)
{
 unsigned long current_pointer;
 unsigned long temp_pointer;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 if (ReadLongName ()==-1)
 {
   #ifdef __DEBUG__
   printf (">>>>* No Material name found\n");
   #endif
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadMaterialChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case MAT_NAME01  :
                          #ifdef __DEBUG__
                          printf (">>>> Found Material def chunk id of %0X\n",temp_int);
                          #endif
                          tellertje+=ReadMatDefChunk ();
                          break;
        default:break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadEditChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case EDIT_UNKNW01:tellertje+=ReadUnknownChunk (EDIT_UNKNW01);break;
        case EDIT_UNKNW02:tellertje+=ReadUnknownChunk (EDIT_UNKNW02);break;
        case EDIT_UNKNW03:tellertje+=ReadUnknownChunk (EDIT_UNKNW03);break;
        case EDIT_UNKNW04:tellertje+=ReadUnknownChunk (EDIT_UNKNW04);break;
        case EDIT_UNKNW05:tellertje+=ReadUnknownChunk (EDIT_UNKNW05);break;
        case EDIT_UNKNW06:tellertje+=ReadUnknownChunk (EDIT_UNKNW06);break;
        case EDIT_UNKNW07:tellertje+=ReadUnknownChunk (EDIT_UNKNW07);break;
        case EDIT_UNKNW08:tellertje+=ReadUnknownChunk (EDIT_UNKNW08);break;
        case EDIT_UNKNW09:tellertje+=ReadUnknownChunk (EDIT_UNKNW09);break;
        case EDIT_UNKNW10:tellertje+=ReadUnknownChunk (EDIT_UNKNW10);break;
        case EDIT_UNKNW11:tellertje+=ReadUnknownChunk (EDIT_UNKNW11);break;
        case EDIT_UNKNW12:tellertje+=ReadUnknownChunk (EDIT_UNKNW12);break;
        case EDIT_UNKNW13:tellertje+=ReadUnknownChunk (EDIT_UNKNW13);break;

        case EDIT_MATERIAL :
                            #ifdef __DEBUG__
                            printf (">>> Found Materials chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadMaterialChunk ();
                            break;
        case EDIT_VIEW1    :
                            #ifdef __DEBUG__
                            printf (">>> Found View main def chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadViewChunk ();
                            break;
        case EDIT_BACKGR   :
                            #ifdef __DEBUG__
                            printf (">>> Found Backgr chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadBackgrChunk ();
                            break;
        case EDIT_AMBIENT  :
                            #ifdef __DEBUG__
                            printf (">>> Found Ambient chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadAmbientChunk ();
                            break;
        case EDIT_OBJECT   :
                            #ifdef __DEBUG__
                            printf (">>> Found Object chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadObjectChunk ();
                            break;
        default:            break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadKeyfChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case KEYF_UNKNWN01 :tellertje+=ReadUnknownChunk (temp_int);break;
        case KEYF_UNKNWN02 :tellertje+=ReadUnknownChunk (temp_int);break;
        case KEYF_FRAMES   :
                            #ifdef __DEBUG__
                            printf (">>> Found Keyframer frames chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadUnknownChunk (temp_int);
                            break;
        case KEYF_OBJDES   :
                            #ifdef __DEBUG__
                            printf (">>> Found Keyframer object description chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadUnknownChunk (temp_int);
                            break;
        case EDIT_VIEW1    :
                            #ifdef __DEBUG__
                            printf (">>> Found View main def chunk id of %0X\n",temp_int);
                            #endif
                            tellertje+=ReadViewChunk ();
                            break;
        default:            break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
     end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
unsigned long ReadMainChunk (void)
{
 unsigned char end_found=FALSE;
 unsigned int temp_int;
 unsigned long current_pointer;
 unsigned long temp_pointer;
 unsigned long tellertje=6L;

 current_pointer=GetChunkPointer ();
 temp_pointer   =ReadChunkPointer ();

 while (end_found==FALSE)
 {
   temp_int=ReadInt ();

       switch (temp_int)
       {
        case KEYF3DS :
                      #ifdef __DEBUG__
                      printf (">> Found *Keyframer* chunk id of %0X\n",KEYF3DS);
                      #endif
                      tellertje+=ReadKeyfChunk ();
                      break;
        case EDIT3DS :
                      #ifdef __DEBUG__
                      printf (">> Found *Editor* chunk id of %0X\n",EDIT3DS);
                      #endif
                      tellertje+=ReadEditChunk ();
                      break;
        default:      break;
       }

   tellertje+=2;
   if (tellertje>=temp_pointer)
    end_found=TRUE;
 }

 ChangeChunkPointer (current_pointer+temp_pointer); // move to the new chunk position
 return (temp_pointer);
}
/*----------------------------------------------------------------------------*/
int ReadPrimaryChunk (void)
{
 unsigned char version;

 if (ReadInt ()==MAIN3DS)
 {
  #ifdef __DEBUG__
  printf ("> Found Main chunk id of %0X\n",MAIN3DS);
  #endif
  //>---------- find version number
  fseek (bin3ds,28L,SEEK_SET);
  version=ReadChar ();
  if (version<3)
  {
   #ifdef __DEBUG__
   printf ("Sorry this lib can only read 3ds files of version 3.0 and higher\n");
   printf ("The version of the file you want to read is: %d\n",version);
   #endif
   return (1);
  }
  fseek (bin3ds,2,SEEK_SET);
  ReadMainChunk ();
 }
 else
  return (1);

 return (0);
}
/*----------------------------------------------------------------------------*/
/*                      Test Main for the 3ds-bin lib                         */
/*----------------------------------------------------------------------------*/
int main (int argc,char **argv)
{
 argc=argc;

 bin3ds=fopen (argv [1],"rb");
 if (bin3ds==NULL)
  return (-1);

 #ifdef __DEBUG__
 printf ("\nLoading 3ds binary file : %s\n",argv [1]);
 #endif
 while (ReadPrimaryChunk ()==0);

 return (0);
}
/*----------------------------------------------------------------------------*/
#endif
