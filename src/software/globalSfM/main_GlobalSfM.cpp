
// Copyright (c) 2012, 2013, 2014 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdlib>

#include "software/globalSfM/SfMGlobalEngine.hpp"
using namespace openMVG;

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"
#include "openMVG/system/timer.hpp"

int main(int argc, char **argv)
{
  using namespace std;
  std::cout << std::endl
    << "-----------------------------------------------------------\n"
    << "Global Structure from Motion:\n"
    << "-----------------------------------------------------------\n"
    << "Open Source implementation of the paper:\n"
    << "\"Global Fusion of Relative Motions for "
    << "Robust, Accurate and Scalable Structure from Motion.\"\n"
    << "Pierre Moulon, Pascal Monasse and Renaud Marlet. "
    << " ICCV 2013." << std::endl
    << "------------------------------------------------------------"
    << std::endl;


  CmdLine cmd;

  std::string sSfM_Data_Filename;
  std::string sMatchesDir;
  std::string sOutDir = "";
  bool bColoredPointCloud = false;
  int iRotationAveragingMethod = 2;
  int iTranslationAveragingMethod = 1;
  bool bRefineFocalAndPP = true;
  bool bRefineDisto = true;

  cmd.add( make_option('i', sSfM_Data_Filename, "input_file") );
  cmd.add( make_option('m', sMatchesDir, "matchdir") );
  cmd.add( make_option('o', sOutDir, "outdir") );
  cmd.add( make_option('c', bColoredPointCloud, "coloredPointCloud") );
  cmd.add( make_option('r', iRotationAveragingMethod, "rotationAveraging") );
  cmd.add( make_option('t', iTranslationAveragingMethod, "translationAveraging") );
  cmd.add( make_option('f', bRefineFocalAndPP, "refineFocalAndPP") );
  cmd.add( make_option('d', bRefineDisto, "refineDisto") );

  try {
    if (argc == 1) throw std::string("Invalid parameter.");
    cmd.process(argc, argv);
  } catch(const std::string& s) {
    std::cerr << "Usage: " << argv[0] << '\n'
    << "[-i|--input_file] path to a SfM_Data scene\n"
    << "[-m|--matchdir] path to the matches that corresponds to the provided SfM_Data scene\n"
    << "[-o|--outdir] path where the output data will be stored\n"
    << "[-c|--coloredPointCloud 0(default) or 1]\n"
    << "[-r|--rotationAveraging 2(default L2) or 1 (L1)]\n"
    << "[-t|--translationAveraging 1(default L1) or 2 (L2)]\n"
    << "[-f|--refineFocalAndPP \n"
    << "\t 0-> keep provided focal and principal point,\n"
    << "\t 1-> refine provided focal and principal point ] \n"
    << "[-d|--refineDisto \n"
    << "\t 0-> refine focal and principal point\n"
    << "\t 1-> refine focal, principal point and radial distortion factors.] \n"
    << "\n"
    << " ICCV 2013: => -r 2 -t 1"
    << std::endl;

    std::cerr << s << std::endl;
    return EXIT_FAILURE;
  }

  if (iRotationAveragingMethod < ROTATION_AVERAGING_L1 ||
      iRotationAveragingMethod > ROTATION_AVERAGING_L2 )  {
    std::cerr << "\n Rotation averaging method is invalid" << std::endl;
    return EXIT_FAILURE;
  }

    if (iTranslationAveragingMethod < TRANSLATION_AVERAGING_L1 ||
      iTranslationAveragingMethod > TRANSLATION_AVERAGING_L2 )  {
    std::cerr << "\n Translation averaging method is invalid" << std::endl;
    return EXIT_FAILURE;
  }

  if (sOutDir.empty())  {
    std::cerr << "\nIt is an invalid output directory" << std::endl;
    return EXIT_FAILURE;
  }

  if (!stlplus::folder_exists(sOutDir))
    stlplus::folder_create(sOutDir);

  //---------------------------------------
  // Incremental reconstruction process
  //---------------------------------------

  openMVG::Timer timer;
  GlobalReconstructionEngine to3DEngine(
    sSfM_Data_Filename,
    sMatchesDir,
    sOutDir,
    ERotationAveragingMethod(iRotationAveragingMethod),
    ETranslationAveragingMethod(iTranslationAveragingMethod),
    true);

  to3DEngine.setRefineFocalAndPP(bRefineFocalAndPP);
  to3DEngine.setRefineDisto(bRefineDisto);

  if (to3DEngine.Process())
  {
    std::cout << std::endl << " Total Ac-Global-Sfm took (s): " << timer.elapsed() << std::endl;

    //-- Compute color if requested
    const reconstructorHelper & reconstructorHelperRef = to3DEngine.refToReconstructorHelper();
    std::vector<Vec3> vec_tracksColor;
    if (bColoredPointCloud)
    {
      // Compute the color of each track
      to3DEngine.ColorizeTracks(to3DEngine.getTracks(), vec_tracksColor);
    }

    //-- Export computed data to disk
    reconstructorHelperRef.exportToPly(
      stlplus::create_filespec(sOutDir, "FinalColorized", ".ply"),
      bColoredPointCloud ? &vec_tracksColor : NULL);

    // Export to openMVG format
    std::cout << std::endl << "Export 3D scene to openMVG format" << std::endl
      << " -- Point cloud color: " << (bColoredPointCloud ? "ON" : "OFF") << std::endl;

    if (!to3DEngine.getFilenamesVector().empty())
    {
      const std::string sImaDirectory = stlplus::folder_part(to3DEngine.getFilenamesVector()[0]);
      if (!reconstructorHelperRef.ExportToOpenMVGFormat(
        stlplus::folder_append_separator(sOutDir) + "SfM_output",
        to3DEngine.getFilenamesVector(),
        sImaDirectory,
        to3DEngine.getImagesSize(),
        to3DEngine.getTracks(),
        bColoredPointCloud ? &vec_tracksColor : NULL,
        true,
        std::string("generated by the Global OpenMVG Calibration Engine")))
      {
        std::cerr << "Error while saving the scene." << std::endl;
      }
    }

    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
