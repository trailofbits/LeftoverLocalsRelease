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
//
//  CovertListenerApp.swift
//  CovertListener


import SwiftUI
import MetalKit


class AppState: ObservableObject {
    // UI variables
    @Published var secretPhrase: String = ""
    @Published var foundData: String = ""
    @Published var errorStr: String = ""
    @Published var searchPrefix: Bool = false
    @Published var isShowingProgress: Bool = false
    @Published var searchOnRefocus: Bool = false
    
    // GPU boilerplate
    @Published var device:MTLDevice? = nil
    @Published var library:MTLLibrary? = nil
    @Published var kernel:MTLFunction? = nil
    @Published var commandQueue:MTLCommandQueue? = nil
    
    // GPU kernel launching variables
    @Published var threadsPerGroup = 1024
    @Published var TGPerGrid = 32

    // GPU memory buffers
    // TODO initialize buffer to 0
    @Published var buffer:MTLBuffer? = nil
    @Published var localMemorySIZE_INT = 1024*8
    
    // hack to get background thread to wait for main
    @State var waitingForMain:Bool = false
    
    func initGPU() -> Void {
        if device == nil {
            // apparently have to update these in the main thread...
            // make the device in the main thread, and then wait for
            // it to finish in this thread. This pattern appears throughout
            DispatchQueue.main.sync {
                self.device = MTLCreateSystemDefaultDevice()
            }
            
            if device == nil {
                DispatchQueue.main.async {
                    self.errorStr += "could not create device\n"
                }
                return
            }
        }
        if library == nil {
            
            DispatchQueue.main.sync {
                self.library =  self.device?.makeDefaultLibrary()
            }
            
            if library == nil {
                DispatchQueue.main.async {
                    self.errorStr += "Could not create default library\n"
                }
                return
            }
        }
        

        
        if kernel == nil {
            
            DispatchQueue.main.sync {
                self.kernel =  self.library?.makeFunction(name: "covertListenerKernel")
            }
            
            if kernel == nil {
                DispatchQueue.main.async {
                    self.errorStr += "Could not create kernel\n"
                }
                return
            }
        }
        
        if buffer == nil {
            let memSize = localMemorySIZE_INT * TGPerGrid
            DispatchQueue.main.sync {
                self.buffer = self.device?.makeBuffer(length:memSize * MemoryLayout<Int32>.stride , options: [.storageModeShared])
            }
            
            if buffer == nil {
                DispatchQueue.main.async {
                    self.errorStr += "Could not create buffer\n"
                }
                return
            }
        }
        buffer!.contents().initializeMemory(as: Int32.self, to: 0)
        
        if commandQueue == nil {
            DispatchQueue.main.sync {
                self.commandQueue = self.device?.makeCommandQueue()
            }
            
            if commandQueue == nil {
                DispatchQueue.main.async {
                    self.errorStr += "Could not create command queue\n"
                }
                return
            }
            
        }
    }
    
    func runListener() -> Void {
        
        var commandBuffer = commandQueue?.makeCommandBuffer()
        if  commandBuffer == nil {
            DispatchQueue.main.async {
                self.errorStr += "Could not create command buffer\n"
            }
            return
        }
        
        
        var encoder = commandBuffer?.makeComputeCommandEncoder()!
        if encoder == nil {
            DispatchQueue.main.async {
                self.errorStr += "Could not create encoder\n"
            }
            return
        }
        
        encoder?.setBuffer(buffer, offset: 0, index: 0)
        
        var pipelineState: MTLComputePipelineState? = nil
        
        do {
            pipelineState = try device?.makeComputePipelineState(function: kernel!)
        }
        catch {
            DispatchQueue.main.async {
                self.errorStr += "Could not create pipeline state\n"
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
                self.errorStr += "error running kernel: " + e!.localizedDescription + "\n"
            }
            return
        }
    }
    
    func checkData() -> Void {
       // assert(searchPrefix == false, "does not support searching for prefix at this time")
        
       
        
        if (searchPrefix) {
            let memSize = localMemorySIZE_INT * TGPerGrid
            let b = buffer!.contents().bindMemory(to: UInt32.self, capacity: memSize)
            var c: [Character] = []
            for i in 0..<(memSize) {
                let q: UInt32 = UInt32(b[i])
                let s = UnicodeScalar(q)
                if s != nil {
                    c.append(Character(s!))
                }
            }
            var s = String(c)
            var message = ""
            while message == ""  {
                
                let r = s.range(of: secretPhrase)
                if r == nil {
                    DispatchQueue.main.async {
                        self.isShowingProgress = false
                        self.foundData +=  "Could not find first secret phrase\n"
                    }
                    return
                }
                let chopped = s[r!.upperBound...]
                let r2 = chopped.range(of: secretPhrase)
                
                if r2 == nil {
                    DispatchQueue.main.async {
                        self.isShowingProgress = false
                        self.foundData +=  "Could not find second secret phrase\n"
                    }
                    return
                }
                
                let message = chopped[..<r2!.lowerBound]
                //if message == "" {
                //    print(s)
                //}
                if message != "" {
                    DispatchQueue.main.async {
                        self.isShowingProgress = false
                        self.foundData +=  "Found message!\n" + message
                    }
                    return
                }
                s = String(chopped)
                if s == "" {
                    DispatchQueue.main.async {
                        self.isShowingProgress = false
                        self.foundData +=  "Could not find message\n" + message
                        
                    }
                    return
                }
            }
            
        }
        else {
            
            let memSize = localMemorySIZE_INT * TGPerGrid
            let b = buffer!.contents().bindMemory(to: Int32.self, capacity: memSize)
            var observations: [Int32: Int32] = [:]
            for i in 0..<(memSize) {
                if observations.keys.contains(b[i]) {
                    observations[b[i]]! += 1
                }
                else {
                    observations[b[i]] = 1
                }
            }
            var l: [(Int32, Int32)] = []
            for k in observations.keys {
                l.append((k,observations[k]!))
            }
            let ll = l.sorted(by: {$0.1 > $1.1})
            
            DispatchQueue.main.async {
                self.isShowingProgress = false
                self.foundData +=  ll.description + "\n"
            }
        }
        
    }
    
    func searchForData() -> Void {
        initGPU()
        runListener()
        checkData()
    }
}

@main
struct CovertListenerApp: App {
    
    @Environment(\.scenePhase) private var scenePhase
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            ContentView().environmentObject(appState)
        }.onChange(of: scenePhase) { newScenePhase in
            if newScenePhase == .active {
                if (appState.searchOnRefocus) {
                    if appState.isShowingProgress == true {
                        return
                    }
                    appState.foundData = ""
                    appState.isShowingProgress = true
                    let dwi = DispatchWorkItem {
                        appState.searchForData()
                        
                    }
                    DispatchQueue.global().async(execute: dwi)
                }
                // Function to be executed when the app enters the foreground
            }
        }
    }
}
