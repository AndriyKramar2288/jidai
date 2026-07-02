<template>
    <div v-if="msg && msg.payload && msg.payload.anki" class="flex flex-col items-center gap-2 w-full" :style="{
            '--text': (msg?.payload?.theme?.text || '#FFFFFF'),
            '--rgb0': (msg?.payload?.theme?.rgb_hex || '#000000'),
            '--rgb1': (msg?.payload?.theme?.rgb_hex || '#000000') + '44',
            '--rgb2': (msg?.payload?.theme?.rgb_hex || '#000000') + '99',
            '--rgb3': (msg?.payload?.theme?.rgb_hex || '#000000') + '22'
        }">
        <div class="flex gap-2">
            <div class="w-2 bg-[var(--rgb1)] rounded-xs"></div>
            <h1 class="text-3xl font-bold text-[var(--text)] px-4 py-2 bg-[var(--rgb1)] rounded-xs">
                Anki
            </h1>
            <div class="w-2 bg-[var(--rgb1)] rounded-xs"></div>
        </div>

        <div class="flex p-10 gap-4 flex-wrap justify-center">
            <div class="bg-[var(--rgb3)] rounded-sm p-5">
                Залишилось: <span class="font-extrabold">{{ msg.payload.anki.totalRemaining }}</span>
            </div>
            <div class="bg-[var(--rgb3)] border-b-8 border-blue-300 rounded-sm p-5">
                Нові: <span class="font-extrabold">{{ msg.payload.anki.totalNew }}</span>
            </div>
            <div class="bg-[var(--rgb3)] border-b-8 border-green-300 rounded-sm p-5">
                Повторити: <span class="font-extrabold">{{ msg.payload.anki.totalDue }}</span>
            </div>
        </div>

        <div class="w-40 h-2 rounded-xs bg-[var(--rgb2)]"></div>

        <div v-if="decks" class="w-full flex items-center justify-center gap-3 mt-4">
            <button @click="prev" class="px-2 py-1 bg-[var(--rgb1)] rounded-sm">‹</button>

            <div class="w-[320px] overflow-hidden">
                <div class="flex transition-transform duration-300"
                    :style="{ transform: `translateX(-${index * 100}%)` }">
                    <div v-for="(deck, i) in decks" :key="i"
                        class="min-w-full bg-[var(--rgb3)] rounded-sm p-4 flex flex-col gap-2">
                        <div class="font-bold text-[var(--text)] truncate">
                            {{ deck.name }}
                        </div>
                        <div class="flex items-center justify-between text-sm opacity-80">
                            <span class="w-10 h-10 flex justify-center items-center rounded-sm bg-blue-300 text-black font-bold">{{ deck.new }}</span>
                            <div class="flex gap-2">
                                <span
                                    class="w-6 h-6 rounded-sm"
                                    :class="(deck.new === 0) ? 'bg-green-400' : 'bg-[var(--rgb1)]'"
                                ></span>
                                <span
                                    class="w-6 h-6 rounded-sm"
                                    :class="(deck.new === 0 && deck.due === 0) ? 'bg-green-400' : 'bg-[var(--rgb1)]'"
                                ></span>
                                <span
                                    class="w-6 h-6 rounded-sm"
                                    :class="(deck.due === 0) ? 'bg-green-400' : 'bg-[var(--rgb1)]'"
                                ></span>
                            </div>
                            <span class="w-10 h-10 flex justify-center items-center rounded-sm bg-green-300 text-black font-bold">{{ deck.due }}</span>
                        </div>
                    </div>
                </div>
            </div>

            <button @click="next" class="px-2 py-1 bg-[var(--rgb1)] rounded-sm">›</button>
        </div>
    </div>
</template>

<script>
    export default {
    props: ["msg"],
    data() {
        return {
            index: 0,
            timer: null
        }
    },
    computed: {
        decks() {
            return this.msg?.payload?.anki?.decks || []
        }
    },
    methods: {
        next() {
            if (!this.decks.length) return
            this.index = (this.index + 1) % this.decks.length
        },
        prev() {
            if (!this.decks.length) return
            this.index = (this.index - 1 + this.decks.length) % this.decks.length
        }
    },
    mounted() {
        this.timer = setInterval(() => this.next(), 5000)
    },
    beforeUnmount() {
        clearInterval(this.timer)
    }
}
</script>