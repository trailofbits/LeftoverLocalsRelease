clspv covertListener.cl
spirv-opt --strip-nonsemantic a.spv -o stripped.spv
mv stripped.spv ../covertVKListener/app/src/main/assets/stripped.spv 

clspv covertWriter.cl
spirv-opt --strip-nonsemantic a.spv -o stripped.spv
mv stripped.spv ../covertVKWriter/app/src/main/assets/stripped.spv

rm a.spv
