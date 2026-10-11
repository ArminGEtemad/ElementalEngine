// moving from the recursion calculation on CPU to gpu compute

static const uint MAX_SEGMENTS_PER_STRIKE = 2048;

struct Segments {
  float3 p0;
  float scale;
  float3 p1;
  float pad;
};

struct DrawIndirectCommand {
  uint vertexCount;
  uint instanceCount;
  uint firstVertex;
  uint firstInstance;
};

struct StrikeComputeConfig {
  float displacement;
  uint maxGenerations;
  float2 pad;
};

struct LightningComputeConstants {
  float3 startPoint;
  float pad0;
  float3 endPoint;
  uint randomSeed;
  StrikeComputeConfig configs[4];
};

[[vk::push_constant]] LightningComputeConstants lightningConstants;

RWStructuredBuffer<Segments> outSegments : register(u0);
RWStructuredBuffer<DrawIndirectCommand> indirectCmds : register(u1);

// David Hoskins Hash function
float hash11(float p1) {
  float p = frac(p1 * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return frac(p);
}

// Emulate a recursive frame stack
struct StackItem {
  float3 startP;
  float3 endP;
  float displace;
  uint generation;
  float scale;
};

//  1 Thread Group containing 4 Threads for 4 strikes that occur with pressing
//  space
[numthreads(4, 1, 1)] void CSMain(uint3 DTid : SV_DispatchThreadID) {
  uint strikeIndex = DTid.x;
  if (strikeIndex >= 4)
    return;

  // Each strike gets a dedicated block in the mega buffer
  uint baseMemoryOffset = strikeIndex * MAX_SEGMENTS_PER_STRIKE;
  uint segmentCounter = 0;

  StrikeComputeConfig config = lightningConstants.configs[strikeIndex];

  // Create our Thread-Local Stack
  StackItem stack[64];
  int stackPtr = 0;

  // Push the very first generation (the root)
  stack[0].startP = lightningConstants.startPoint;
  stack[0].endP = lightningConstants.endPoint;
  stack[0].displace = config.displacement;
  stack[0].generation = 0;
  stack[0].scale = 1.0f;
  stackPtr++;

  float idSeed =
      float(lightningConstants.randomSeed) + (float)strikeIndex * 13.0f;
  uint iterations = 0;

  // replaces the recursion.
  while (stackPtr > 0 && segmentCounter < MAX_SEGMENTS_PER_STRIKE &&
         iterations < 5000) {
    iterations++;
    stackPtr--;

    StackItem current = stack[stackPtr];

    if (current.generation >= config.maxGenerations) {
      Segments seg;
      seg.p0 = current.startP;
      seg.scale = current.scale;
      seg.p1 = current.endP;
      outSegments[baseMemoryOffset + segmentCounter] = seg;
      segmentCounter++;
      continue;
    }

    // Midpoint math
    float3 mid = (current.startP + current.endP) * 0.5f;
    float3 dir = current.endP - current.startP;
    float len = length(dir);

    if (len > 1e-5f) {
      float3 mainDir = dir / len;

      float s1 = (hash11(idSeed + iterations * 1.1f) * 2.0f) - 1.0f;
      float s2 = (hash11(idSeed + iterations * 2.2f) * 2.0f) - 1.0f;
      float s3 = (hash11(idSeed + iterations * 3.3f) * 2.0f) - 1.0f;

      float3 randomVec = normalize(float3(s1, s2, s3));
      if (abs(dot(mainDir, randomVec)) > 0.99f)
        randomVec = float3(1.0f, 0.0f, 0.0f);

      float3 perp = normalize(cross(mainDir, randomVec));
      float offsetAmount =
          ((hash11(idSeed + iterations * 4.4f) * 2.0f) - 1.0f) *
          current.displace;
      mid += perp * offsetAmount;
    }

    // Forking Logic
    if (current.generation == 2 || current.generation == 3) {

      float forkRoll = hash11(idSeed + iterations * 5.5f);

      if (forkRoll < 0.50f) { // 50% chance to fork a sister branch
        float3 tDir = current.endP - mid;
        float tLen = length(tDir);

        if (tLen > 1.0f) {
          float3 mainDir = tDir / tLen;

          float angleSign =
              (hash11(idSeed + iterations * 6.6f) > 0.5f) ? 1.0f : -1.0f;

          float angle = angleSign *
                        (20.0f + hash11(idSeed + iterations * 7.7f) * 0.25f) *
                        0.01745f;

          float3 randomAxis = normalize(
              float3((hash11(idSeed + iterations * 8.8f) * 2.0f) - 1.0f,
                     (hash11(idSeed + iterations * 9.9f) * 2.0f) - 1.0f,
                     (hash11(idSeed + iterations * 10.1f) * 2.0f) - 1.0f));

          if (abs(dot(mainDir, randomAxis)) > 0.99f)
            randomAxis = float3(1.0f, 0.0f, 0.0f);
          float3 rotAxis = normalize(cross(mainDir, randomAxis));

          float cosA = cos(angle);
          float sinA = sin(angle);
          float3 branchDir = mainDir * cosA + cross(rotAxis, mainDir) * sinA +
                             rotAxis * dot(rotAxis, mainDir) * (1.0f - cosA);

          float3 branchEnd = mid + branchDir * tLen;
          if (stackPtr < 63) {
            stack[stackPtr].startP = mid;
            stack[stackPtr].endP = branchEnd;
            stack[stackPtr].displace = current.displace * 0.5f;
            stack[stackPtr].scale = current.scale * 0.35f;
            stack[stackPtr].generation = current.generation + 1;
            stackPtr++;
          }
        }
      }
    }

    if (stackPtr < 62) {
      // Segment 2 (mid to end)
      stack[stackPtr].startP = mid;
      stack[stackPtr].endP = current.endP;
      stack[stackPtr].displace = current.displace * 0.5f;
      stack[stackPtr].scale = current.scale;
      stack[stackPtr].generation = current.generation + 1;
      stackPtr++;

      // Segment 1 (start to mid)
      stack[stackPtr].startP = current.startP;
      stack[stackPtr].endP = mid;
      stack[stackPtr].displace = current.displace * 0.5f;
      stack[stackPtr].scale = current.scale;
      stack[stackPtr].generation = current.generation + 1;
      stackPtr++;
    }
  }

  DrawIndirectCommand cmd;
  cmd.vertexCount = segmentCounter * 6;
  cmd.instanceCount = 1;
  cmd.firstVertex = baseMemoryOffset * 6;
  cmd.firstInstance = 0;

  indirectCmds[strikeIndex] = cmd;
}