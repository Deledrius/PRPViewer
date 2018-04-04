#include "Age.hpp"
#include "helpers/Logger.hpp"
#include <string>
#include <ResManager/plResManager.h>
#include <Debug/hsExceptions.hpp>
#include <Stream/hsStdioStream.h>
#include <Stream/hsStream.h>
#include <PRP/plSceneNode.h>
#include <PRP/Object/plSceneObject.h>
#include <PRP/Object/plDrawInterface.h>
#include <PRP/Object/plCoordinateInterface.h>
#include <PRP/Geometry/plDrawableSpans.h>
#include <PRP/Surface/plLayer.h>
#include <PRP/Surface/plBitmap.h>
#include <PRP/Surface/plMipmap.h>
#include <Stream/plEncryptedStream.h>
#include <Debug/plDebug.h>
#include <Util/plPNG.h>
#include <Util/plJPEG.h>
#include <string_theory/stdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <fstream>

/*
static void pl_png_write(png_structp png, png_bytep data, png_size_t size) {
	hsStream* S = reinterpret_cast<hsStream*>(png_get_io_ptr(png));
	S->write(size, reinterpret_cast<const uint8_t*>(data));
}

void SavePNG(hsStream* S, const void* buf, size_t size,
						uint32_t width, uint32_t height, int pixelSize) {
	png_structp fPngWriter;
	png_infop   fPngInfo;
	
	
	
	fPngWriter = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!fPngWriter)
		throw hsPNGException(__FILE__, __LINE__, "Error initializing PNG writer");
	
	fPngInfo = png_create_info_struct(fPngWriter);
	if (!fPngInfo) {
		png_destroy_write_struct(&fPngWriter, NULL);
		fPngWriter = NULL;
		throw hsPNGException(__FILE__, __LINE__, "Error initializing PNG info structure");
	}
	png_set_write_fn(fPngWriter, (png_voidp)S, &pl_png_write, NULL);
	png_set_IHDR(fPngWriter, fPngInfo, width, height, 8,
				 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	// Plasma uses BGR for DirectX (TODO: Does HSPlasma need this?)
	//png_set_bgr(pi.fPngWriter);
	
	png_write_info(fPngWriter, fPngInfo);
	
	png_bytep src = reinterpret_cast<png_bytep>(const_cast<void*>(buf));
	std::unique_ptr<png_bytep[]> rows(new png_bytep[height]);
	const unsigned int stride = width * pixelSize / 8;
	for (size_t i = 0; i < height; ++i) {
		rows[i] = src + (i * stride);
		if (rows[i] + stride > src + size)
			throw hsPNGException(__FILE__, __LINE__, "PNG input buffer is too small");
	}
	
	png_write_image(fPngWriter, rows.get());
	png_write_end(fPngWriter, fPngInfo);
	png_destroy_write_struct(&fPngWriter, &fPngInfo);
}

*/


Age::Age(const std::string & path){
	
	const PlasmaVer plasmaVersion = PlasmaVer::pvMoul;
	_rm.reset(new plResManager());
	
	plAgeInfo* age = _rm->ReadAge(path, true);
	
	_name = age->getAgeName().to_std_string();
	
	Log::Info() << "Age " << _name << ":" << std::endl;
	
	const size_t pageCount = age->getNumPages();
	Log::Info() << pageCount << " pages." << std::endl;
	for(int i = 0 ; i < pageCount; ++i){
		Log::Info() << "Page: " << age->getPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getPageLoc(i, plasmaVersion);
		//loadMeshes(*_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
	const size_t commmonCount = age->getNumCommonPages(plasmaVersion);
	Log::Info() << commmonCount << " common pages." << std::endl;
	for(int i = 0 ; i < commmonCount; ++i){
		Log::Info() << "Page: " << age->getCommonPageFilename(i, plasmaVersion) << " ";
		const plLocation ploc = age->getCommonPageLoc(i, plasmaVersion);
		//loadMeshes(*_rm, ploc);
		Log::Info() << std::endl;
	}
	Log::Info() << std::endl;
	
}


void Age::loadMeshes(plResManager & rm, const plLocation& ploc){
	plSceneNode* scene = rm.getSceneNode(ploc);
	if(scene){
	
		//Log::Info() << scene->getKey()->getName() << " (" << scene->getSceneObjects().size() << ")" << std::endl;
		
		for(const auto & objKey : scene->getSceneObjects()){
			// For each object, store its subgeometries (icicle)
			// and the used materials, referencing textures.
			
			plSceneObject* obj = plSceneObject::Convert(rm.getObject(objKey));
			if(!obj->getDrawInterface().Exists()){
				continue;
			}
			
			
			plDrawInterface* draw = plDrawInterface::Convert(obj->getDrawInterface()->getObj());
			plCoordinateInterface* coord = NULL;
			if (obj->getCoordInterface().Exists()){
				coord = plCoordinateInterface::Convert(obj->getCoordInterface()->getObj());
			}
			Log::Info() << "Object: " << objKey->getName() << ": ";
			Log::Info() << draw->getNumDrawables() << " drawables." << std::endl;
			for (size_t i = 0; i < draw->getNumDrawables(); ++i) {
				if (draw->getDrawableKey(i) == -1){
					continue;
				}
			
				// A span group multiple objects data/assets.
				plDrawableSpans* span = plDrawableSpans::Convert(draw->getDrawable(i)->getObj());
				plDISpanIndex di = span->getDIIndex(draw->getDrawableKey(i));
				// Ignore matrix-only span element. probably internal nodes ?
				if ((di.fFlags & plDISpanIndex::kMatrixOnly) != 0){
					continue;
				}
				Log::Info() << "\tIn drawable span " << i << " (" << span->getKey()->getName() << "): " << di.fIndices.size() << " indices." << std::endl;
					
				for(size_t id = 0; id < di.fIndices.size(); ++id){
					const std::string fileName = scene->getKey()->getName().to_std_string() + "__" + obj->getKey()->getName().to_std_string() + "__" + std::to_string(i) + "_" + std::to_string(id) ;
//					std::ofstream outfile(outputPath + "/" + fileName + ".obj");
//					if(!outfile.is_open()){
//						std::cerr << "ISSUE" << std::endl;
//						continue;
//					}
					
					
					// Get the mesh internal representation.
					plIcicle* ice = span->getIcicle(di.fIndices[id]);
					Log::Info() << "\t\t" << "Icicle " << id << " (" << di.fIndices[id] << ")";
					//Log::Info() << span->getSpan(di.fIndices[id])
					Log::Info() << std::endl;
					
					
					
					std::vector<plGBufferVertex> verts = span->getVerts(ice);
					std::vector<unsigned short> indices = span->getIndices(ice);
					//Log::Info() << "Num uvs: " << span->getBuffer(ice->getGroupIdx())->getNumUVs() << std::endl;
					for (size_t j = 0; j < verts.size(); j++) {
						hsVector3 pos;
						if (coord != NULL){
							pos = coord->getLocalToWorld().multPoint(verts[j].fPos);// * 10.0f;
						} else {
							pos = ice->getLocalToWorld().multPoint(verts[j].fPos);// * 10.0f;
						}
					//	outfile << "v " << pos.X << " " << pos.Z << " " << (-pos.Y) << std::endl;
						
					//	outfile << "vn " <<  verts[j].fNormal.X << " " <<  verts[j].fNormal.Z << " " << (- verts[j].fNormal.Y) << std::endl;
						
					}
					
					bool hasTexture = false;
					// Find proper uv transformation (we can translate/scale material layers)
					
					plKey matKey = span->getMaterials()[ice->getMaterialIdx()];
					if (matKey.Exists()) {
						hsGMaterial* mat = hsGMaterial::Convert(matKey->getObj(), false);
						if (mat == NULL || mat->getLayers().empty()) {
							continue;
						}
						
						Log::Info() << "\t\t\tMaterial: " << mat->getKey()->getName();
						Log::Info() << ", " << mat->getLayers().size() << " layers. ";
						Log::Info() << "Compo: " << mat->getCompFlags() << ", piggyback: " << mat->getPiggyBacks().size() << std::endl;
						
						for(size_t lid = 0; lid < mat->getLayers().size(); ++lid){
							plLayerInterface* lay = plLayerInterface::Convert(mat->getLayers()[lid]->getObj(), false);
							Log::Info() << "\t\t\t\t" << lay->getKey()->getName();
							Log::Info() << ", flags: " << lay->getState().fBlendFlags << " " << lay->getState().fClampFlags << " " <<  lay->getState().fShadeFlags << " " <<  lay->getState().fZFlags  << " " <<  lay->getState().fMiscFlags << ".";
							if(lay->getTexture()){
								Log::Info() << " Texture: " << lay->getTexture()->getName() << ", UV: " << lay->getUVWSrc();
								// Export uvs.
								const unsigned int uvwSrc = lay->getUVWSrc();
								const hsMatrix44 uvwXform = lay->getTransform();
								for (size_t j = 0; j < verts.size(); j++) {
									hsVector3 txUvw = uvwXform.multPoint(verts[j].fUVWs[uvwSrc]);
									// z will probably always be 0
									// -1 for blender?
									//outfile << "vt " << txUvw.X << " " << -txUvw.Y << " " << txUvw.Z << std::endl;
								}
								hasTexture = true;
							}
							Log::Info() << std::endl;
							
						}
							// find lowest underlay UV set.
							// TODO: support undlerlay?
							//						while (lay != NULL && lay->getUnderLay().Exists()){
							//							lay = plLayerInterface::Convert(lay->getUnderLay()->getObj(), false);
							//							Log::Info() << "AZE\t\t\t\t" << lay->getKey()->getName() << std::endl;
							//						}
							
							
					}
						//					if(hasTexture){
						//						for (size_t j = 0; j < verts.size(); j++) {
						//							hsVector3 txUvw = uvwXform.multPoint(verts[j].fUVWs[uvwSrc]);
						//							// z will probably always be 0
						//							// -1 for blender?
						//							outfile << "vt " << txUvw.X << " " << -txUvw.Y << " " << txUvw.Z << std::endl;
						//						}
						//					}
						
						
					for (size_t j = 0; j < indices.size(); j += 3) {
							//outfile << "f " << indices[j]+1 << "/" << (hasTexture ? std::to_string(indices[j]+1) : "") << "/" << indices[j]+1;
							//outfile <<  " " << indices[j+1]+1 << "/" << (hasTexture ? std::to_string(indices[j+1]+1) : "") << "/" << indices[j+1]+1;
							//outfile <<  " " << indices[j+2]+1 << "/" << (hasTexture ? std::to_string(indices[j+2]+1) : "") << "/" << indices[j+2]+1;
							//outfile << std::endl;
					}
						//outfile.close();
						
						
				}
					
			}
		}
	}

	const auto textureKeys = rm.getKeys(ploc, pdUnifiedTypeMap::ClassIndex("plMipmap")); // also support envmap
	Log::Info() << textureKeys.size() << " textures." << std::endl;

	for(const auto & texture : textureKeys){
		// For each texture, send it to the gpu and keep a reference and the name.
		plMipmap* tex = plMipmap::Convert(texture->getObj());
		
		const std::string fileName = "texture__" + texture->getName().to_std_string() ;
		Log::Info() << fileName << std::endl;
		
		
		uint8_t *dest;
		size_t sizeComplete;
		if(tex->getCompressionType() != plBitmap::kDirectXCompression){
			dest = (uint8_t*)(tex->getLevelData(0));
			sizeComplete = tex->getLevelSize(0);
			
		} else {
			size_t blocks = tex->GetUncompressedSize(0) / sizeof(size_t);
			if ((tex->GetUncompressedSize(0)) % sizeof(size_t) != 0)
				++blocks;
			sizeComplete = tex->GetUncompressedSize(0);
			dest = reinterpret_cast<uint8_t*>(new size_t[blocks]);
			//DecompressImage(0, fImageData, fLevelData[0].fSize);
			tex->DecompressImage(0, dest, tex->getLevelSize(0));
			
			
		}
		//hsFileStream outTex;
		//outTex.open(ST::string(outputPath + "/" + fileName + ".png"), FileMode::fmWrite);
		//SavePNG(&outTex, dest, sizeComplete, tex->getLevelWidth(0),  tex->getLevelHeight(0), tex->getBPP());
		//outTex.close();
		
		
	}
}


