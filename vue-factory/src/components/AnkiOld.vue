<template>
    <div
        v-if="msg?.payload?.anki?.stats"
        class="w-full flex flex-col gap-2"
        :style="{
            '--rgb1': (msg?.payload?.theme?.rgb_hex || '#000000') + '44',
            '--rgb2': (msg?.payload?.theme?.rgb_hex || '#000000') + '99',
            '--rgb3': (msg?.payload?.theme?.rgb_hex || '#000000') + '22'
        }"
    >

        <!-- 1. БЛОК ЗАГАЛЬНОЇ СТАТИСТИКИ -->
        <div
            class="bg-[var(--rgb1)] rounded-md p-5 flex flex-col md:flex-row items-start md:items-center justify-between gap-4 border border-[var(--rgb1)] shadow-lg"
        >

            <div class="flex items-center gap-4">

                <!-- Іконка -->
                <div class="p-3 bg-[var(--rgb2)] rounded-sm">
                    <svg
                        class="w-8 h-8 invert-25"
                        fill="var(--rgb2)"
                        viewBox="0 0 24 24"
                        xmlns="http://www.w3.org/2000/svg"
                    >
                        <path
                            d="M4 6H2v14c0 1.1.9 2 2 2h14v-2H4V6zm16-4H8c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm-1 9h-4v4h-2v-4H9V9h4V5h2v4h4v2z"
                            fill="white"
                        />
                    </svg>
                </div>

                <div>
                    <h2 class="text-3xl font-bold text-[var(--rgb2)] invert-25">
                        Anki
                    </h2>

                    <p class="text-amber-50/70 font-medium">
                        Залишилось карток:

                        <span class="text-white font-bold text-lg">
                            {{ msg.payload.anki.stats.totalRemaining }}
                        </span>
                    </p>
                </div>
            </div>

            <!-- Загальні лічильники -->
            <div class="flex gap-2 font-bold">

                <div
                    v-if="msg.payload.anki.stats.totalNew > 0"
                    class="flex flex-col items-center justify-center bg-blue-600/80 border border-blue-400 text-white min-w-[70px] py-1.5 rounded-sm shadow-md"
                >
                    <span class="text-[10px] uppercase opacity-80 tracking-wider">
                        Нові
                    </span>

                    <span class="text-xl leading-none">
                        {{ msg.payload.anki.stats.totalNew }}
                    </span>
                </div>

                <div
                    v-if="msg.payload.anki.stats.totalDue > 0"
                    class="flex flex-col items-center justify-center bg-green-600/80 border border-green-400 text-white min-w-[70px] py-1.5 rounded-sm shadow-md"
                >
                    <span class="text-[10px] uppercase opacity-80 tracking-wider">
                        Повтор
                    </span>

                    <span class="text-xl leading-none">
                        {{ msg.payload.anki.stats.totalDue }}
                    </span>
                </div>

                <div
                    v-if="msg.payload.anki.stats.totalRemaining === 0"
                    class="flex items-center text-white bg-[var(--rgb2)] px-4 py-2 rounded-sm shadow-md"
                >
                    Всі картки пройдені!
                </div>

            </div>
        </div>

        <!-- 2. КОЛОДИ -->
        <div class="grid grid-cols-1 sm:grid-cols-2 gap-3">

            <div
                v-for="(deck, index) in msg.payload.anki.stats.decks"
                :key="index"
                class="group bg-[var(--rgb3)] border border-[var(--rgb1)] hover:bg-[var(--rgb1)] transition-colors duration-300 rounded-md p-4 flex justify-between items-center cursor-default shadow-sm hover:shadow-md"
            >

                <span
                    class="text-lg text-amber-50 truncate pr-4 select-none"
                    :title="deck.name"
                >
                    {{ deck.name }}
                </span>

                <!-- Лічильники -->
                <div class="flex gap-1.5 shrink-0">

                    <div
                        v-if="deck.new > 0"
                        class="w-8 h-8 flex items-center justify-center bg-blue-500 text-white font-bold rounded-md shadow-sm text-sm"
                        title="Нові"
                    >
                        {{ deck.new }}
                    </div>

                    <div
                        v-if="deck.due > 0"
                        class="w-8 h-8 flex items-center justify-center bg-green-500 text-white font-bold rounded-md shadow-sm text-sm"
                        title="Повторити"
                    >
                        {{ deck.due }}
                    </div>

                    <!-- Пусто -->
                    <div
                        v-if="deck.new === 0 && deck.due === 0"
                        class="w-8 h-8 flex items-center justify-center bg-gray-500/30 text-gray-400 rounded-md"
                    >
                        <svg
                            class="w-5 h-5"
                            fill="none"
                            stroke="currentColor"
                            viewBox="0 0 24 24"
                        >
                            <path
                                stroke-linecap="round"
                                stroke-linejoin="round"
                                stroke-width="3"
                                d="M5 13l4 4L19 7"
                            />
                        </svg>
                    </div>

                </div>
            </div>

            <!-- Якщо колод нема -->
            <div
                v-if="
                    !msg.payload.anki.stats.decks ||
                    msg.payload.anki.stats.decks.length === 0
                "
                class="col-span-full p-8 text-center bg-[var(--rgb3)] rounded-md border border-[var(--rgb1)]"
            >
                <p class="text-gray-400 italic text-lg">
                    Немає колод для вивчення
                </p>
            </div>

        </div>

    </div>
</template>

<script>
export default {
    props: ["msg"]
}
</script>