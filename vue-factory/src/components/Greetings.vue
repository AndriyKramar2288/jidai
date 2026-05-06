<template>
    <div class="sepia-50" :style="{ '--rgb1': (msg?.rgb_hex || '#000000') + '44', 
                  '--rgb2': (msg?.rgb_hex || '#000000') + '99', 
                  '--rgb3': (msg?.rgb_hex || '#000000') + '22' }">

        <Transition name="slide-fade" mode="out-in">
            <div v-if="is_opened" key="close" class="flex gap-2">
                <div class="bg-[var(--rgb1)] w-3 rounded-sm"></div>
                <div class="flex flex-col gap-4">
                    <div class="p-6 bg-[var(--rgb1)] rounded-sm flex items-center justify-between">
                        <h1 class="text-3xl font-semibold text-[var(--rgb2)] invert-25">{{msg.header}}</h1>
                        <button @click="is_opened = !is_opened" class="p-3 rounded-sm hover:bg-black/5 duration-75 cursor-pointer">
                            <svg xmlns="http://www.w3.org/2000/svg" class="w-10 h-10 invert-25" fill="var(--rgb2)" width="800px" height="800px" viewBox="-80 0 512 512"><path d="M242.72 256l100.07-100.07c12.28-12.28 12.28-32.19 0-44.48l-22.24-22.24c-12.28-12.28-32.19-12.28-44.48 0L176 189.28 75.93 89.21c-12.28-12.28-32.19-12.28-44.48 0L9.21 111.45c-12.28 12.28-12.28 32.19 0 44.48L109.28 256 9.21 356.07c-12.28 12.28-12.28 32.19 0 44.48l22.24 22.24c12.28 12.28 32.2 12.28 44.48 0L176 322.72l100.07 100.07c12.28 12.28 32.2 12.28 44.48 0l22.24-22.24c12.28-12.28 12.28-32.19 0-44.48L242.72 256z"/></svg>
                        </button>
                    </div>
                    <div class="flex items-center gap-3 px-12">
                        <img class="w-48 rounded-sm shadow-[var(--rgb1)]" src="https://media.tenor.com/MKkJWYigjycAAAAM/cute-cat-cat-cute.gif" alt="cute cat" />
                        <p class="text-xl px-12 text-amber-50">{{msg.text}}</p>
                    </div>
                </div>
            </div>

            <div v-else key="open" class="p-4 border-3 border-[var(--rgb1)] rounded-md">
                <div class="p-16 flex justify-center items-center gap-4 flex-col bg-[var(--rgb1)] rounded-md">
                    <button @click="is_opened = !is_opened" class="cursor-pointer hover:bg-amber-200/10 duration-150 px-8 py-3 rounded-md flex items-center gap-6">
                        <div class="flex flex-col items-stretch gap-1 m-4">
                            <p class="text-3xl text-[var(--rgb2)] invert-25 font-semibold max-w-72">Ковертік щастя, любові та добра</p>
                            <div class="bg-[var(--rgb2)] invert-25 h-1 rounded-md"></div>
                        </div>
                        <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" fill="var(--rgb2)" class="invert-25" height="100px" width="100px" version="1.1" id="Layer_1" viewBox="0 0 512 512" xml:space="preserve"> <g> <g> <path d="M508.65,115.988L363.186,261.451l141.875,141.979c4.295-7.385,6.939-15.85,6.939-24.992V133.563&#13;&#10;&#9;&#9;&#9;C512,127.357,510.722,121.476,508.65,115.988z"/> </g> </g> <g> <g> <path d="M461.913,83.476H50.087c-8.327,0-16.066,2.236-22.982,5.847L245.76,307.976c6.344,6.344,17.363,6.344,23.596,0&#13;&#10;&#9;&#9;&#9;l217.626-217.52C479.58,86.133,471.088,83.476,461.913,83.476z"/> </g> </g> <g> <g> <path d="M4.201,113.611C1.527,119.733,0,126.465,0,133.563v244.875c0,8.997,2.573,17.327,6.743,24.632l143.407-143.51&#13;&#10;&#9;&#9;&#9;L4.201,113.611z"/> </g> </g> <g> <g> <path d="M339.478,285.159l-46.414,46.525c-19.496,19.275-51.084,19.591-70.901,0l-48.306-48.417L32.006,425.017&#13;&#10;&#9;&#9;&#9;c5.625,2.192,11.69,3.507,18.081,3.507h411.826c6.235,0,12.146-1.284,17.657-3.376L339.478,285.159z"/> </g> </g> </svg>
                    </button>     
                </div>
            </div>
        </Transition>
    </div>
</template>
<script>
export default {
    data() {
        return {
            is_opened: false,
            // msg: {
            //     rgb_hex: "#000000",
            //     header: "Мій улюблений сонечко, гарного дня! 🍓",
            //     text: "Навіть якщо за вікном похмуро, пам'ятай, що найяскравіше сонце живе всередині тебе. Пам'ятай, що помилятися — це нормально, головне, що ти продовжуєш йти вперед, і я тобою пишаюся. Смачного тобі обіду і легких завдань сьогодні! ☁️"
            // }
        }
    }
}
</script>
<style>
.slide-fade-enter-active,
.slide-fade-leave-active {
  transition: all 0.1s ease;
}

.slide-fade-enter-from {
  opacity: 0;
  transform: translateY(10px);
}

.slide-fade-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}
</style>