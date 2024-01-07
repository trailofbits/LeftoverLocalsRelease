# WebGPUListener

A simple webpage that launches a WebGPU kernel which dumps local memory. 

## To run
Open `index.html` in a web browser that has up-to-date support for WebGPU. We use Google Chrome (16.0.5845.110) on Mac OS (13.4.1). You can check your GPU by navigating to `chrome://gpu/`

When you open the page, you will see a simple lonely button that says "Run". Click this button to dump local memory. There is no UI for this app. It reports the results in the console, which can be opened with `Option + ⌘ + J` in Mac OS.

You should first see a message:

```
total (leaking is likely if this is more than 0): 0
```

This is simply a sum of all the values dumped from local memory. If anything non-zero was observed, then it is likely that this value will not be 0 (as indicated by the message). 

You will then see a truncated array of unsigned integers, e.g.,

```
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  …]
```

These are the actual values recorded

## Expected Behavior

If a device exhibits LeftoverLocals through WebGPU, we'd expect to see non-zero values here. However, in all of our experiences, we have only observed zeros; which is why we did not implemented a Writer webpage. 

Even GPUs for which we observe LeftoverLocals through other frameworks (e.g., the Apple M2 through Metal), we still observe zeros through WebGPU. This is because WebGPU explicitly initializes local memory, and performs runtime bounds checking. This feature has been [discussed at length](https://github.com/gpuweb/gpuweb/issues/1202), along with the [associated performance overhead](https://github.com/mlc-ai/web-stable-diffusion#comparison-with-native-gpu-runtime-limitations-and-opportunities) (around 3x for stable diffusion). It is possible to launch chrome without these bounds checks with the flag `--enable-dawn-features=disable_robustness`. This flag allows more efficient WebGPU kernel execution, but the GPUs that are vulnerable to LeftoverLocals would now be vulnerable through the WebGPU framework, and thus, an attacker could implement a Listener in a website, which could be widely deployed.

However, WebGPUs default settings, along with strict kernel typing rules appears to fortify devices against LeftoverLocals vulnerabilities. 

You can experiment with WebGPU kernels and see the explicit initialization and bounds checking by building [tint](https://dawn.googlesource.com/tint), and providing it with the WebGPU kernel (e.g., from this program). If you output Metal Shading Language, it is easy to see these protections.

