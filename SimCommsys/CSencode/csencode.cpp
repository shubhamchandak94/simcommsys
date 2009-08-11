#include "serializer_libcomm.h"
#include "commsys.h"
#include "timer.h"

#include <boost/program_options.hpp>
#include <iostream>

namespace csencode {

template <class S, template <class > class C>
void process(const std::string& fname, std::istream& sin = std::cin,
      std::ostream& sout = std::cout)
   {
   // Communication system
   libcomm::commsys<S, C> *system = libcomm::loadfromfile<
         libcomm::commsys<S, C> >(fname);
   std::cerr << system->description() << "\n";
   // Initialize system
   libbase::randgen r;
   r.seed(0);
   system->seedfrom(r);
   // Repeat until end of stream
   while (!sin.eof())
      {
      C<int> source(system->input_block_size());
      source.serialize(sin);
      C<S> transmitted = system->encode_path(source);
      transmitted.serialize(sout, '\n');
      libbase::eatwhite(sin);
      }
   }

/*!
 \brief   Communication Systems Encoder
 \author  Johann Briffa

 \section svn Version Control
 - $Revision$
 - $Date$
 - $Author$
 */

int main(int argc, char *argv[])
   {
   libbase::timer tmain("Main timer");

   // Set up user parameters
   namespace po = boost::program_options;
   po::options_description desc("Allowed options");
   desc.add_options()("help", "print this help message");
   desc.add_options()("system-file,i", po::value<std::string>(),
         "input file containing system description");
   desc.add_options()("type,t",
         po::value<std::string>()->default_value("bool"),
         "modulation symbol type");
   desc.add_options()("container,c", po::value<std::string>()->default_value(
         "vector"), "input/output container type");
   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   // Validate user parameters
   if (vm.count("help") || vm.count("system-file") == 0)
      {
      std::cerr << desc << "\n";
      return 1;
      }
   // Shorthand access for parameters
   const std::string container = vm["container"].as<std::string> ();
   const std::string type = vm["type"].as<std::string> ();
   const std::string filename = vm["system-file"].as<std::string> ();

   // Main process
   if (container == "vector")
      {
      using libbase::vector;
      using libbase::gf;
      using libcomm::sigspace;
      if (type == "bool")
         process<bool, vector> (filename);
      else if (type == "gf2")
         process<gf<1, 0x3> , vector> (filename);
      else if (type == "gf4")
         process<gf<2, 0x7> , vector> (filename);
      else if (type == "gf8")
         process<gf<3, 0xB> , vector> (filename);
      else if (type == "gf16")
         process<gf<4, 0x13> , vector> (filename);
      else if (type == "gf32")
         process<gf<5, 0x25> , vector> (filename);
      else if (type == "gf64")
         process<gf<6, 0x43> , vector> (filename);
      else if (type == "gf128")
         process<gf<7, 0x89> , vector> (filename);
      else if (type == "gf256")
         process<gf<8, 0x11D> , vector> (filename);
      else if (type == "gf512")
         process<gf<9, 0x211> , vector> (filename);
      else if (type == "gf1024")
         process<gf<10, 0x409> , vector> (filename);
      else if (type == "sigspace")
         process<sigspace, vector> (filename);
      else
         {
         std::cerr << "Unrecognized symbol type: " << type << "\n";
         return 1;
         }
      }
   else if (container == "matrix")
      {
      using libbase::matrix;
      using libbase::gf;
      using libcomm::sigspace;
      if (type == "bool")
         process<bool, matrix> (filename);
      else if (type == "gf2")
         process<gf<1, 0x3> , matrix> (filename);
      else if (type == "gf4")
         process<gf<2, 0x7> , matrix> (filename);
      else if (type == "gf8")
         process<gf<3, 0xB> , matrix> (filename);
      else if (type == "gf16")
         process<gf<4, 0x13> , matrix> (filename);
      else if (type == "gf32")
         process<gf<5, 0x25> , matrix> (filename);
      else if (type == "gf64")
         process<gf<6, 0x43> , matrix> (filename);
      else if (type == "gf128")
         process<gf<7, 0x89> , matrix> (filename);
      else if (type == "gf256")
         process<gf<8, 0x11D> , matrix> (filename);
      else if (type == "gf512")
         process<gf<9, 0x211> , matrix> (filename);
      else if (type == "gf1024")
         process<gf<10, 0x409> , matrix> (filename);
      else if (type == "sigspace")
         process<sigspace, matrix> (filename);
      else
         {
         std::cerr << "Unrecognized symbol type: " << type << "\n";
         return 1;
         }
      }
   else
      {
      std::cerr << "Unrecognized container type: " << container << "\n";
      return 1;
      }

   return 0;
   }

}
; // end namespace

int main(int argc, char *argv[])
   {
   return csencode::main(argc, argv);
   }
