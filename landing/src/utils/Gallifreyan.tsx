import { useRef, useEffect } from 'react';

type Point = {
  x: number;
  y: number;
};

type Props = {
  text?: string;
  width?: number;
  height?: number;
  size?: number;
  color?: string;
  thickness?: number;
  lineSpread?: number;
  sentenceSpacing?: number;
  spiralFactor?: number;
  wordSpacing?: number;
};

const PI = Math.PI;

function dist(a: Point, b: Point): number {
  return Math.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2);
}

function normalizeAngle(angle: number): number {
  while (angle > PI) angle -= 2 * PI;
  while (angle < -PI) angle += 2 * PI;
  return angle;
}

function angleBetweenCircles(circle: any, second: any): number {
  const d = dist(circle, second);
  const val = (second.r * second.r - d * d - circle.r * circle.r) / (-2 * d * circle.r);
  return Math.acos(clamp(val, -1, 1));
}

function pointFromAngle(obj: any, r: number, angle: number): [number, number] {
  return [obj.x + Math.cos(angle) * r, obj.y + Math.sin(angle) * r];
}

function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

function segmentIntersectsCircle(x1: number, y1: number, x2: number, y2: number, cx: number, cy: number, r: number) {
  const l2 = (x1 - x2) ** 2 + (y1 - y2) ** 2;
  if (l2 === 0) return dist({ x: x1, y: y1 }, { x: cx, y: cy }) < r;
  let t = ((cx - x1) * (x2 - x1) + (cy - y1) * (y2 - y1)) / l2;
  t = Math.max(0, Math.min(1, t));
  const projX = x1 + t * (x2 - x1);
  const projY = y1 + t * (y2 - y1);
  return dist({ x: cx, y: cy }, { x: projX, y: projY }) < r + 2;
}

function drawCircle(ctx: CanvasRenderingContext2D, x: number, y: number, r: number) {
  ctx.beginPath();
  ctx.arc(x, y, r, 0, PI * 2);
  ctx.stroke();
}

function drawArc(ctx: CanvasRenderingContext2D, x: number, y: number, r: number, a1: number, a2: number) {
  ctx.beginPath();
  ctx.arc(x, y, r, a1, a2);
  ctx.stroke();
}

function drawDot(ctx: CanvasRenderingContext2D, x: number, y: number, r: number) {
  ctx.beginPath();
  ctx.arc(x, y, r, 0, PI * 2);
  ctx.fill();
}

function drawLine(ctx: CanvasRenderingContext2D, x1: number, y1: number, x2: number, y2: number) {
  ctx.beginPath();
  ctx.moveTo(x1, y1);
  ctx.lineTo(x2, y2);
  ctx.stroke();
}

function parseWord(word: string): string[] {
  const result: string[] = [];
  for (let i = 0; i < word.length; i++) {
    const pair = word.substring(i, i + 2);
    if (pair.match(/(ch|sh|th|ng|qu)/)) {
      result.push(pair);
      i++;
    } else if (word[i] === "c") {
      if (i + 1 < word.length && word[i + 1].match(/[iey]/)) {
        result.push("s");
      } else {
        result.push("k");
      }
    } else {
      result.push(word[i]);
    }
  }
  return result;
}

const map: Record<string, number> = {
  b: 1, ch: 2, d: 3, f: 4, g: 5, h: 6,
  j: 1, k: 2, l: 3, m: 4, n: 5, p: 6,
  t: 1, sh: 2, r: 3, s: 4, v: 5, w: 6,
  th: 1, y: 2, z: 3, ng: 4, qu: 5, x: 6,
  a: 1, e: 2, i: 3, o: 4, u: 5
};

export default function Gallifreyan({
  text = "hello world",
  width,
  height,
  size = 800,
  color = "#000000",
  thickness = 3,
  lineSpread = 0.25,
  sentenceSpacing = 1,
  spiralFactor = 0.7,
  wordSpacing = 0,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !text.trim()) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const w = width || size;
    const h = height || size;
    const minDim = Math.min(w, h);

    const dpr = window.devicePixelRatio || 1;
    canvas.width = w * dpr;
    canvas.height = h * dpr;
    canvas.style.width = `${w}px`;
    canvas.style.height = `${h}px`;

    ctx.scale(dpr, dpr);
    ctx.clearRect(0, 0, w, h);

    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineWidth = thickness;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    const allCircles: any[] = [];
    const midX = w / 2;
    const midY = h / 2;

    class Circle {
      x!: number;
      y!: number;
      a!: number;
      d!: number;

      owner: any;
      children: Circle[] = [];
      type: number;
      subtype: number;
      r: number;

      isWord: boolean;
      isLetter: boolean;
      isVowel: boolean;
      isConsonant: boolean;
      hasGaps: boolean;
      clearBg: boolean = false;

      dots: number;
      nLines = 0;

      constructor(owner: any, type: number, subtype: number, d: number, r: number, a: number) {
        this.owner = owner;
        this.type = type;
        this.subtype = subtype;
        this.r = r;

        this.isWord = this.subtype === 0;
        this.isLetter = this.subtype > 0;
        this.isVowel = this.type === 5 || this.type === 6;
        this.isConsonant = !this.isVowel && this.isLetter;
        this.hasGaps = this.type === 1 || this.type === 3;

        this.dots = this.isConsonant ? [0, 2, 3, 0, 0, 0][this.subtype - 1] || 0 : 0;

        this.update(d, a);
      }

      update(d: number, a: number) {
        const oldA = this.a;
        const [x, y] = pointFromAngle(this.owner || { x: midX, y: midY }, d, a);
        this.x = x;
        this.y = y;
        this.d = d;
        this.a = normalizeAngle(a);

        if (oldA !== undefined) {
          let diff = this.a - oldA;
          if (diff > PI) diff -= 2 * PI;
          if (diff < -PI) diff += 2 * PI;

          for (const child of this.children) {
            child.update(child.d, child.a + diff);
          }
        }
      }

      draw(ctx: CanvasRenderingContext2D) {
        if (this.clearBg) {
          ctx.save();
          ctx.globalCompositeOperation = 'destination-out';
          ctx.beginPath();
          ctx.arc(this.x, this.y, this.r, 0, PI * 2);
          ctx.fill();
          ctx.restore();
        }

        if (this.isWord) {
          let gaps: number[][] = [];
          
          // 1. Розриви від власних літер (Type 1, 3, 4)
          for (const child of this.children) {
            if (child.type === 1 || child.type === 3 || child.type === 4) {
              const an = angleBetweenCircles(this, child);
              if (!isNaN(an)) {
                const angToChild = Math.atan2(child.y - this.y, child.x - this.x);
                let start = angToChild - an;
                let end = angToChild + an;
                const PI2 = PI * 2;
                start = (start % PI2 + PI2) % PI2;
                end = (end % PI2 + PI2) % PI2;

                if (start > end) {
                  gaps.push([start, PI2]);
                  gaps.push([0, end]);
                } else {
                  gaps.push([start, end]);
                }
              }
            }
          }

          // 2. НОВЕ: Розриви від стикування/перекриття з іншими словами
          if (this.owner && this.owner.children) {
            for (const sibling of this.owner.children) {
              if (sibling !== this && sibling.isWord) {
                const myIdx = this.owner.children.indexOf(this);
                const sibIdx = this.owner.children.indexOf(sibling);
                
                // Якщо сусід малюється ПІСЛЯ нас (має більший індекс), він "зверху", тож ми робимо виріз
                if (sibIdx > myIdx) {
                  const d = dist(this, sibling);
                  const gapMargin = thickness * 1.5; // Візуальний зазор між словами
                  const sibR = sibling.r + gapMargin;

                  // Якщо кола перетинаються
                  if (d < this.r + sibR && d > 0) {
                    const val = (sibR * sibR - d * d - this.r * this.r) / (-2 * d * this.r);
                    if (val >= -1 && val <= 1) {
                      const an = Math.acos(val);
                      const angToSib = Math.atan2(sibling.y - this.y, sibling.x - this.x);
                      let start = angToSib - an;
                      let end = angToSib + an;
                      const PI2 = PI * 2;
                      start = (start % PI2 + PI2) % PI2;
                      end = (end % PI2 + PI2) % PI2;

                      if (start > end) {
                        gaps.push([start, PI2]);
                        gaps.push([0, end]);
                      } else {
                        gaps.push([start, end]);
                      }
                    }
                  }
                }
              }
            }
          }

          if (gaps.length === 0) {
            drawCircle(ctx, this.x, this.y, this.r);
          } else {
            gaps.sort((a, b) => a[0] - b[0]);
            let merged: number[][] = [[...gaps[0]]];
            for (let i = 1; i < gaps.length; i++) {
              let last = merged[merged.length - 1];
              let current = gaps[i];
              if (current[0] <= last[1] + 0.001) {
                last[1] = Math.max(last[1], current[1]);
              } else {
                merged.push([...current]);
              }
            }

            let currentStart = 0;
            for (let gap of merged) {
              if (gap[0] > currentStart + 0.001) {
                drawArc(ctx, this.x, this.y, this.r, currentStart, gap[0]);
              }
              currentStart = gap[1];
            }
            if (currentStart < PI * 2 - 0.001) {
              drawArc(ctx, this.x, this.y, this.r, currentStart, PI * 2);
            }
          }
        } else if (this.isLetter && this.hasGaps && this.owner) {
          const an = angleBetweenCircles(this, this.owner);
          if (!isNaN(an)) {
            const angToOwner = Math.atan2(this.owner.y - this.y, this.owner.x - this.x);
            drawArc(ctx, this.x, this.y, this.r, angToOwner - an, angToOwner + an);
          } else {
            drawCircle(ctx, this.x, this.y, this.r);
          }
        } else {
          drawCircle(ctx, this.x, this.y, this.r);
        }

        if (this.dots) {
          let currentDotR = 3 + thickness / 2;
          let orbitR = Math.max(this.r * 0.4, this.r - 10);
          let overlap = true;
          let attempts = 40;

          while (overlap && attempts > 0) {
            overlap = false;
            for (let i = 0; i < this.dots; i++) {
              const p1 = pointFromAngle(this, orbitR, this.a + i);
              for (let j = i + 1; j < this.dots; j++) {
                const p2 = pointFromAngle(this, orbitR, this.a + j);
                if (dist({ x: p1[0], y: p1[1] }, { x: p2[0], y: p2[1] }) < currentDotR * 2 + 1.5) {
                  overlap = true;
                  break;
                }
              }
              if (overlap) break;
            }
            if (overlap) {
              currentDotR *= 0.9;
            }
            attempts--;
          }

          for (let i = 0; i < this.dots; i++) {
            drawDot(ctx, ...pointFromAngle(this, orbitR, this.a + i), currentDotR);
          }
        }

        if (this.nLines > 0 && this.owner?.r !== undefined) {
          const word = this.owner;
          for (let i = 0; i < this.nLines; i++) {
            let baseOffset = (i - (this.nLines - 1) / 2) * lineSpread;
            let finalX1 = 0, finalY1 = 0, finalX2 = 0, finalY2 = 0;
            let validLineFound = false;

            for (let tryIdx = 0; tryIdx < 150; tryIdx++) {
              let deviation = (tryIdx === 0) ? 0 : (tryIdx % 2 === 0 ? 1 : -1) * Math.floor((tryIdx + 1) / 2) * 0.05;
              let testOffset = baseOffset + deviation;
              let rayAngle = this.a + PI + testOffset;

              let x1 = this.x + Math.cos(rayAngle) * this.r;
              let y1 = this.y + Math.sin(rayAngle) * this.r;
              let x2 = word.x + Math.cos(rayAngle) * word.r;
              let y2 = word.y + Math.sin(rayAngle) * word.r;

              let collision = false;
              if (word.isWord) {
                for (const child of word.children) {
                  if (child !== this) {
                    if (dist(this, child) < Math.max(this.r, child.r)) continue;
                    if (segmentIntersectsCircle(x1, y1, x2, y2, child.x, child.y, child.r)) {
                      collision = true;
                      break;
                    }
                  }
                }
              }

              if (!collision) {
                finalX1 = x1;
                finalY1 = y1;
                finalX2 = x2;
                finalY2 = y2;
                validLineFound = true;
                break;
              }
            }

            if (!validLineFound) {
              let rayAngle = this.a + PI + baseOffset;
              finalX1 = this.x + Math.cos(rayAngle) * this.r;
              finalY1 = this.y + Math.sin(rayAngle) * this.r;
              finalX2 = word.x + Math.cos(rayAngle) * word.r;
              finalY2 = word.y + Math.sin(rayAngle) * word.r;
            }

            drawLine(ctx, finalX1, finalY1, finalX2, finalY2);
          }
        }
      }
    }

    const sentencesRaw = text.trim().toLowerCase().split(/(?<=[.!?])\s+|\n+/).filter(Boolean);
    const sentences = sentencesRaw.map(s => {
      return s.replace(/[.!?]/g, '').trim().split(/\s+/).filter(Boolean).map(parseWord);
    }).filter(s => s.length > 0);

    const N = sentences.length;

    const D = 3.5 / Math.max(0.1, sentenceSpacing);

    let bestDx = D / Math.sqrt(2);
    if (N > 1) {
      let lo = 0.01;
      let hi = D - 0.01;
      const targetRatio = w / h;

      for (let iter = 0; iter < 40; iter++) {
        let mid = (lo + hi) / 2;
        let dyTest = Math.sqrt(D * D - mid * mid);
        let bw = (N - 1) * mid + 2;
        let bh = dyTest + 2;
        if (bw / bh < targetRatio) lo = mid;
        else hi = mid;
        bestDx = mid;
      }
    }

    const dx = bestDx;
    const dy = Math.sqrt(Math.max(0, D * D - dx * dx));
    const Bw = (N > 1) ? (N - 1) * dx + 2 : 2;
    const Bh = dy + 2;

    const S = 0.95 * Math.min(w / Bw, h / Bh);
    const R = S;
    const innerR = R * 0.92;

    const sentenceGeoms = sentences.map((sentence, i) => {
      const nx = i * dx;
      const ny = (i % 2 === 0) ? dy : 0;

      const cx = midX + (nx - (N - 1) * dx / 2) * S;
      const cy = midY + (ny - dy / 2) * S;

      const deltaX = cx - midX;
      const deltaY = cy - midY;
      const d = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
      const sAngle = Math.atan2(deltaY, deltaX);

      return {
        x: cx, y: cy, r: R, innerR: innerR, d, a: sAngle,
        wordsData: sentence,
        bestPositions: [] as { r: number, a: number }[],
        currentWordRadius: 0
      };
    });

    function computeSpiral(
      numWords: number,
      wordR: number,
      iR: number,
      pitchMul: number,
      extraSpacing: number
    ): { r: number, a: number }[] | null {
      // Зменшено множник до 1.75 для дозволу часткового перекриття
      const arcSpacing = wordR * 1.75 + extraSpacing;
      const pitch      = wordR * 1.8 * pitchMul;
      const rStart     = iR * 0.93 - wordR;
      const rMin       = wordR * 1.05;

      if (rStart < rMin) return null;

      const positions: { r: number, a: number }[] = [];
      let cR         = rStart;
      let cAngle     = 0;
      let totalSwept = 0;

      for (let wIdx = 0; wIdx < numWords; wIdx++) {
        if (cR < rMin) return null;
        positions.push({ r: cR, a: cAngle });

        const dTheta = arcSpacing / Math.max(cR, rMin);
        totalSwept  += dTheta;
        cAngle      -= dTheta;
        cR           = rStart - (pitch / (2 * PI)) * totalSwept;
      }
      return positions;
    }

    for (let i = 0; i < sentenceGeoms.length; i++) {
      const sg    = sentenceGeoms[i];
      const words = sg.wordsData;
      if (words.length === 0) continue;

      const extraSpacing = (wordSpacing / 100) * minDim;
      const pitchMul = Math.max(1.1, 1.1 + spiralFactor * 2.4);

      let lo = sg.innerR * 0.005;
      let hi = sg.innerR * 0.48;
      let bestWordR = lo;
      let bestRaw: { r: number, a: number }[] | null = null;

      for (let iter = 0; iter < 40; iter++) {
        const mid = (lo + hi) / 2;
        const raw = computeSpiral(words.length, mid, sg.innerR, pitchMul, extraSpacing);
        if (raw) { bestWordR = mid; bestRaw = raw; lo = mid; }
        else      { hi = mid; }
      }

      if (!bestRaw) {
        bestWordR = sg.innerR * 0.05;
        const delta = (2 * PI) / words.length;
        bestRaw = words.map((_, wIdx) => ({
          r: sg.innerR * 0.6,
          a: -wIdx * delta
        }));
      }

      const overlappingSentences = sentenceGeoms.filter((other, j) => {
        if (j <= i) return false; 
        return dist({ x: sg.x, y: sg.y }, { x: other.x, y: other.y }) < sg.r + other.r;
      });

      const globalOffset = PI / 2;

      const positions: { r: number, a: number }[] = bestRaw.map(p => ({
        r: p.r,
        a: normalizeAngle(p.a + globalOffset)
      }));

      const safeMargin = minDim * 0.002;
      const minR = bestWordR * 1.05;
      const maxR = sg.innerR * 0.93 - bestWordR;

      for (let pass = 0; pass < 35; pass++) {
        for (let wIdx = 0; wIdx < positions.length; wIdx++) {
          let { r, a } = positions[wIdx];
          
          let px = sg.x + Math.cos(a) * r;
          let py = sg.y + Math.sin(a) * r;

          for (const other of overlappingSentences) {
            const needed = bestWordR + other.r + safeMargin + extraSpacing; 
            const d = dist({ x: px, y: py }, { x: other.x, y: other.y });
            if (d < needed && d > 0.001) {
              const overlap = needed - d;
              const dx = (px - other.x) / d;
              const dy = (py - other.y) / d;

              px += dx * overlap;
              py += dy * overlap;
            }
          }

          for (let k = 0; k < positions.length; k++) {
            if (k === wIdx) continue;
            const other = positions[k];
            const ox = sg.x + Math.cos(other.a) * other.r;
            const oy = sg.y + Math.sin(other.a) * other.r;
            
            // Зменшили коефіцієнт відштовхування до 1.75 для ефекту "стикування"
            const needed = bestWordR * 1.75 + safeMargin + extraSpacing; 
            const d = dist({ x: px, y: py }, { x: ox, y: oy });
            if (d < needed && d > 0.001) {
              const overlap = needed - d;
              const dx = (px - ox) / d;
              const dy = (py - oy) / d;

              px += dx * overlap * 0.5;
              py += dy * overlap * 0.5;

              let nx = ox - dx * overlap * 0.5;
              let ny = oy - dy * overlap * 0.5;
              
              let nr = Math.sqrt((nx - sg.x) ** 2 + (ny - sg.y) ** 2);
              let na = Math.atan2(ny - sg.y, nx - sg.x);
              nr = Math.max(minR, Math.min(maxR, nr));
              
              positions[k] = { r: nr, a: na };
            }
          }

          r = Math.sqrt((px - sg.x) ** 2 + (py - sg.y) ** 2);
          a = Math.atan2(py - sg.y, px - sg.x);

          r = Math.max(minR, Math.min(maxR, r));

          positions[wIdx] = { r, a };
        }
      }

      sg.currentWordRadius = bestWordR;
      sg.bestPositions = positions;
    }

    const paragraphCircle = new Circle(null, 4, -1, 0, Math.max(w, h), 0);
    allCircles.push(paragraphCircle);

    for (let i = 0; i < sentenceGeoms.length; i++) {
      const sg = sentenceGeoms[i];
      const words = sg.wordsData;

      const sentenceOuter = new Circle(paragraphCircle, 4, -1, sg.d, sg.r, sg.a);
      sentenceOuter.x = sg.x;
      sentenceOuter.y = sg.y;
      sentenceOuter.clearBg = true;
      paragraphCircle.children.push(sentenceOuter);
      allCircles.push(sentenceOuter);

      const sentenceInner = new Circle(sentenceOuter, 4, -1, 0, sg.innerR, 0);
      sentenceOuter.children.push(sentenceInner);
      allCircles.push(sentenceInner);

      for (let wIdx = 0; wIdx < words.length; wIdx++) {
        const word = words[wIdx];
        const wPos = sg.bestPositions[wIdx] || { r: 0, a: 0 };

        const mainCircle = new Circle(sentenceInner, 2, 0, wPos.r, sg.currentWordRadius, wPos.a);
        sentenceInner.children.push(mainCircle);
        allCircles.push(mainCircle);

        let lAngle = PI / 2;
        const lDelta = (2 * PI) / word.length;
        const letterR = (1.8 * sg.currentWordRadius) / (word.length + 2);

        for (const letter of word) {
          const subtype = map[letter];
          if (!subtype) continue;

          let type = 2;
          let lr = letterR;
          let ld = sg.currentWordRadius - lr;

          if (letter.match(/^(b|ch|d|f|g|h)$/)) {
            type = 1;
            ld = sg.currentWordRadius - lr * 0.65;
          } else if (letter.match(/^(j|k|l|m|n|p)$/)) {
            type = 2;
            ld = sg.currentWordRadius - lr;
          } else if (letter.match(/^(t|sh|r|s|v|w)$/)) {
            type = 3;
            ld = sg.currentWordRadius + lr * 0.35;
          } else if (letter.match(/^(th|y|z|ng|qu|x)$/)) {
            type = 4;
            ld = sg.currentWordRadius;
          } else if (letter.match(/^[aeiou]$/)) {
            type = 5;
            lr = letterR * 0.6;
            if (letter === 'a') {
              ld = sg.currentWordRadius + lr * 1.2;
            } else if (letter === 'o') {
              ld = sg.currentWordRadius - letterR * 1.2;
            } else {
              ld = sg.currentWordRadius;
            }
          }

          const circle = new Circle(mainCircle, type, subtype, ld, lr, lAngle);
          circle.nLines = [0, 0, 0, 3, 1, 2][subtype - 1] || 0;

          mainCircle.children.push(circle);
          allCircles.push(circle);

          lAngle = normalizeAngle(lAngle - lDelta);
        }
      }
    }

    for (const c of allCircles) {
      if (c !== paragraphCircle) c.draw(ctx);
    }

  }, [text, width, height, size, color, thickness, lineSpread, sentenceSpacing, spiralFactor, wordSpacing]);

  return <canvas ref={canvasRef} />;
}