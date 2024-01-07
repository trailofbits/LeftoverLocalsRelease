# A LeftoverLocals PoC LLM Attack

This repo shows how LeftoverLocals can be exploited by a co-resident attacker that can listen to another users LLM chat session. More details are provided in the LeftoverLocals write-up.

## Attacker Assumptions:
1. The attacker is [co-resident](https://www.educative.io/answers/how-co-residency-attack-is-achieved) on the same machine with the victim.
2. The target machine is using a GPU that is vulnerable to LeftoverLocals
3. The victim is using the GPU to optimize an interactive chat session with an LLM.
4. The attacker has fingerprinted the model that the victim is using (either using LeftoverLocals or using [another method](https://arxiv.org/pdf/1810.03487.pdf)).
5. The model is available, e.g., through Hugging Face.

### The Model
We assume that the model is `wizardLM-7B.ggmlv3.q5_0.bin`, which is available from [Hugging Face](https://huggingface.co/TheBloke/wizardLM-7B-GGML) (but not included in the repo for size purposes). The attack would work with other models, but a few changes to the code are required to account for different output layer dimensions.

Please download the model and place it in the `models` directory in `llama.cpp`.

### Changes to llama.cpp
We assume that the victim is using llama.cpp, but the attack should work with any DNN if the attacker is able to fingerprint the model. 

Our llama.cpp contains one small change. Originally, llama.cpp implements their own matrix-vector multiplicaiton for the OpenCL backend. This is a suboptimal kernel, and we replace it with a more optimal kernel. For example, [this blog post](http://www.bealto.com/gpu-gemv_intro.html) benchmarks several approaches. The current llama.cpp uses the V2 approach; we replace it with an approach more similar to V3, which was reported to be 1.3 to 2x faster. This is achieved by caching the vector in local memory. Because of this, it turns out that the V3 kernel is also easier for an attacker to listen to, although, it would not be impossible with the existing V2.

This function is implemented in ggml-opencl.cpp as `ToB_optimized_matvecmult`.

## Executing the Attack

### Preparing the Victim
The victim process should navigate into the `llama.cpp` directory. They should build llama.cpp with the OpenCL backend using the instructions provided with llama.cpp. There are some dependencies (e.g., CLBLAST). After the dependencies are optained, llama.cpp can be built by running:

```
make LLAMA_CLBLAST=1 -j8
```

After that, the victim should launch an interactive chat session, e.g., by running:

```
./main -m ./models/wizardLM-7B.ggmlv3.q5_0.bin -ngl 128 -n 256 --repeat_penalty 1.0 --color -i -r "User:" -f prompts/chat-with-bob.txt -mg 1
```

Please note that this command tells llama.cpp to use the GPU for all layers (at least for this model). Running this command should start a chat session with some example text being written:

```
== Running in interactive mode. ==
 - Press Ctrl+C to interject at any time.
 - Press Return to return control to LLaMa.
 - To return control without starting a new line, end your input with '/'.
 - If you want to submit another line, end your input with '\'.

 Transcript of a dialog, where the User interacts with an Assistant named Bob. Bob is helpful, kind, honest, good at writing, and never fails to answer the User's requests immediately and with precision.

User: Hello, Bob.
Bob: Hello. How may I help you today?
User: Please tell me the largest city in Europe.
Bob: Sure. The largest city in Europe is Moscow, the capital of Russia.
User:
```


### Preparing the Attacker
The attacker also utilizes the code in the `llama.cpp` directory, but it doesn't have to be the same directory as the victim. The code could be in a different user or even different docker container. 

The attacker should build llama.cpp with the OpenCL backend; the same as the victim. However, this also builds a new program: clCovertListener. This program is very similar to the OpenCLCLI LeftoverLocals project with a few changes so that it listens to the LLM. It takes in many of the same commandline options (device selection, number of workgroups, size of workgroups, etc. Just run with `-h` to see).

The attacker should then run this program just by running `./clCovertListener`. This attack program uses the llama.cpp framework for the model utilities (e.g., loading it from the file and performing some computation), so it will output the normal llama.cpp configuration messages. At that point it will wait. 

### The attack
Make sure that the victim is running an interactive LLM and the attacker has launched the clCovertListener program. Make sure they are using the same GPU and that the GPU is vulnerable to LeftoverLocals.

In the victim process, type in a query and press enter. For example, we might ask the LLM: "tell me about Trail of Bits". To which it might respond:

```
User:Tell me about Trail of Bits
Bob: Trail of Bits is a security firm based in New York City that specializes in cybersecurity consulting and threat intelligence. They have a strong reputation in the industry for providing high-quality services and solutions to their clients.
User:
```

In the attacker process, you should be able to see a large portion of the output of the query. For example, when we run it on an AMD Radeon RX 7900 XT, we see the following output

```
 Rule: Trail of Bits is a cy firm company that in New York City that specializes in cybersecurity.ing and threat intelligence. They have a strong reputation for the industry for providing high-quality services and to expert to their clients.
```

There is some missing text, and a few nonsense words (especially at the beginning), but overall we can see that the output of the previous LLM query is produced with high accuracy. This listening can continue to occur with the attacker continuing to spy on the victims interactive LLM chat.

### Behind the Scenes

The way that this attack works is that the listener continually dumps local memory. Because it has fingerprinted the model, it knows that the output layer is a matrix-vector multiplication with dimensions (32001,4096) x (4096,1) where the matrix is part of the model. Again, because the model has been fingerprinted and obtained, the attacker can simply use these model weights.

To obtain the vector, we use the optimized matrix-vector multiplication, which caches the vector in local memory for each workgroup. We found that for the AMD GPU, this would put 4096 floating point values seperated by zeros. Thus, we search for that specific pattern in our local memory dumps. Once it is found, it executes the output layer computation directly. The result of the multiplication can be fed mapped back to text using the models tokenizer.

## Resources

A video narrating the process is provided in LeftoverLocalsPoCLLMAttack.mp4
