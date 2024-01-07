/*
   Copyright 2023 Trail of Bits

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//  ContentView.swift
//  Covert_Listener


import SwiftUI
import MetalKit

struct ContentView: View {
    
    // UI variables
    @State private var secretPhrase: String = ""
    @State private var ToWrite: String = ""
    @State private var errorStr: String = ""
    @State private var isShowingProgress: Bool = false
    @State private var endWrite: Bool = false
    
    // GPU boilerplate
    @State var device:MTLDevice? = nil
    @State var library:MTLLibrary? = nil
    @State var kernel:MTLFunction? = nil
    @State var commandQueue:MTLCommandQueue? = nil
    
    // GPU kernel launching variables
    @State var threadsPerGroup = 1024
    @State var TGPerGrid = 32

    // GPU memory buffers
    @State var buffer:MTLBuffer? = nil
    @State var sizeBuffer:MTLBuffer? = nil
    @State var fakeBuffer:MTLBuffer? = nil
    let localMemorySIZE_INT = 1024*8
    @State var messageLength = 0
    

    
    // memory sizes
    
    func initGPU() -> Void {
        if device == nil {
            device = MTLCreateSystemDefaultDevice()
            if device == nil {
                DispatchQueue.main.async {
                    errorStr += "could not create device\n"
                }
                return
            }
        }
        
        if library == nil {
            library =  device?.makeDefaultLibrary()
            if library == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create default library\n"
                }
                return
            }
        }
        
        if kernel == nil {
            kernel =  library?.makeFunction(name: "covertListenerKernel")
            if kernel == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create kernel\n"
                }
                return
            }
        }
        
        if buffer == nil {
            let memSize = localMemorySIZE_INT * TGPerGrid
            buffer = device?.makeBuffer(length:memSize * MemoryLayout<Int32>.stride , options: [.storageModeShared])
            if buffer == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create buffer\n"
                }
                return
            }
        }
        buffer!.contents().initializeMemory(as: Int32.self, to: 0)
        
        if sizeBuffer == nil {
            sizeBuffer = device?.makeBuffer(length:1 * MemoryLayout<Int32>.stride , options: [.storageModeShared])
            if sizeBuffer == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create sizeBuffer\n"
                }
                return
            }
        }
        sizeBuffer!.contents().initializeMemory(as: Int32.self, to: 0)
        
        if fakeBuffer == nil {
            fakeBuffer = device?.makeBuffer(length:1 * MemoryLayout<Int32>.stride , options: [.storageModeShared])
            if fakeBuffer == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create fakeBuffer\n"
                }
                return
            }
        }
        fakeBuffer!.contents().initializeMemory(as: Int32.self, to: 0)
        
        if commandQueue == nil {
            commandQueue = device?.makeCommandQueue()
            if commandQueue == nil {
                DispatchQueue.main.async {
                    errorStr += "Could not create command queue\n"
                }
                return
            }
            
        }
    }
    
    func runWriter() -> Void {
        
        var commandBuffer = commandQueue?.makeCommandBuffer()
        if  commandBuffer == nil {
            DispatchQueue.main.async {
                errorStr += "Could not create command buffer\n"
            }
            return
        }
        
        
        var encoder = commandBuffer?.makeComputeCommandEncoder()!
        if encoder == nil {
            DispatchQueue.main.async {
                errorStr += "Could not create encoder\n"
            }
            return
        }
        
        encoder?.setBuffer(buffer, offset: 0, index: 0)
        encoder?.setBuffer(sizeBuffer, offset: 0, index: 1)
        encoder?.setBuffer(fakeBuffer, offset: 0, index: 2)


        
        var pipelineState: MTLComputePipelineState? = nil
        
        do {
            pipelineState = try device?.makeComputePipelineState(function: kernel!)
        }
        catch {
            DispatchQueue.main.async {
                errorStr += "Could not create pipeline state\n"
            }
            return
        }
        
        encoder!.setComputePipelineState(pipelineState!)
        
        @State var threadsPerGroupSIZE = MTLSize(width: threadsPerGroup, height: 1, depth: 1)
        @State var TGPerGridSOZE = MTLSize(width: TGPerGrid, height: 1, depth: 1)
        
        encoder!.dispatchThreadgroups(TGPerGridSOZE, threadsPerThreadgroup: threadsPerGroupSIZE)
        
        // This is important
        encoder!.endEncoding()
        
        commandBuffer!.commit()
        commandBuffer!.waitUntilCompleted()
        
        let e = commandBuffer!.error
        
        if e != nil {
            DispatchQueue.main.async {
                errorStr += "error running kernel: " + e!.localizedDescription + "\n"
            }
            return
        }
    }
    
    func populateMessage() -> Bool {
        let memSize = localMemorySIZE_INT * TGPerGrid
        let b = buffer!.contents().bindMemory(to: UInt32.self, capacity: memSize)
        let bs = sizeBuffer!.contents().bindMemory(to: UInt32.self, capacity: 1)
        if secretPhrase == "" ||  ToWrite == ""{
            DispatchQueue.main.async {
                errorStr += "Please set a secret key or set a message to write!\n"
            }
            return false
        }
        let asciiValues = secretPhrase.compactMap { $0.asciiValue }
        var index = 0
        for a in asciiValues {
            //print(a)
            b[index] = UInt32(a)
            index += 1
        }
        //print("end secret message")
        
        let asciiValues2 = ToWrite.compactMap { $0.asciiValue }

        for a in asciiValues2 {
            //print(a)
            b[index] = UInt32(a)
            index += 1
        }
        //print("end message message")
        
        for a in asciiValues {
            //print(a)
            b[index] = UInt32(a)
            index += 1
        }

        
        messageLength = index
        bs[0] = UInt32(messageLength)
        /*print("start")
        for i in 0..<index {
            print(b[i])
        }
        print("end")*/

        return true
        
    }
    
    func writeData() -> Bool {
        initGPU()
        if !populateMessage() {
            return false
        }
        runWriter()
        return true
    }


    var body: some View {
        VStack {
           
            Text("Covert Writer").font(.system(size: 24)).padding()
            Button("Start Write") {
                if isShowingProgress == true {
                    return
                }
                isShowingProgress = true
                let dwi = DispatchWorkItem {
                    var localEndWrite = false
                    errorStr = ""
                    while !localEndWrite && writeData() {
                        DispatchQueue.main.async {
                            localEndWrite = endWrite
                        }
                    }
                    endWrite = false
                    isShowingProgress = false

                }

                DispatchQueue.global().async(execute: dwi)
                
            }.fontWeight(.bold)
                .font(.title)
                .foregroundColor(.purple)
                .padding()
                .border(Color.purple, width: 5)
            Button("End Write") {
                endWrite = true
                
            }.fontWeight(.bold)
                .font(.title)
                .foregroundColor(.purple)
                .padding()
                .border(Color.purple, width: 5)
            TextField("Secret Key", text: $secretPhrase).padding()
                .border(Color.purple, width: 5)
            TextField("Message to write", text: $ToWrite).padding()
                .border(Color.red, width: 5).lineLimit(10)
            TextField("Error log", text: $errorStr).padding()
                .border(Color.yellow, width: 5)
            if isShowingProgress {
                            ProgressView()
                                .progressViewStyle(CircularProgressViewStyle())
                                .padding()
                        }
               
        }
        .padding()
        Spacer()
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}


