<template>
    <div  :style="{ '--rgb1': (msg?.rgb_hex || '#000000') + '44', 
                    '--rgb2': (msg?.rgb_hex || '#000000') + '99', 
                    '--rgb3': (msg?.rgb_hex || '#000000') + '22' }">

        <div class="flex">
            <div class="w-1 rounded-l-sm rounded-br-sm bg-[var(--rgb1)]"></div>
            <div class="">
                <div class="bg-[var(--rgb1)] px-4 py-6 rounded-r-sm">
                    <h2 class="text-3xl font-bold text-[var(--rgb1)] invert-25">Протоколи</h2>
                </div>
                <ul class="m-12 flex flex-col gap-3 list-none">
                    <li v-for="(protocol, index) in availableProtocols" :key="index">
                        <label class="flex items-center justify-between gap-4 cursor-pointer">

                        <!-- hidden checkbox -->
                        <input
                            type="checkbox"
                            class="sr-only peer"
                            v-model="selectedProtocols"
                            :value="protocol"
                            @change="handleProtocolToggle(protocol)" 
                        />

                        <!-- switch -->
                        <div
                            class="w-11 h-6 flex items-center bg-[var(--rgb1)] rounded-md p-1
                                peer-checked:bg-orange-500 transition-colors duration-300"
                        >
                            <div
                                class="w-4 h-4 bg-orange-300 peer-checked:bg-[var(--rgb1)] rounded-md shadow-md transform transition-transform duration-300"
                                :class="{ 'translate-x-5': selectedProtocols.includes(protocol) }"
                            ></div>
                        </div>

                        <!-- label -->
                        <span class="text-xl select-none">
                            {{ protocol }}
                        </span>

                        </label>
                    </li>
                </ul>
                <div class="bg-[var(--rgb1)] h-1px px-4 py-6 rounded-r-sm">
                </div>
            </div>
        </div>
    </div>
</template>

<script>
    export default {
        data() {
            return {
                availableProtocols: ["weqwq", "wqqwqwe", "wqeewqqweqwe"],
                selectedProtocols: [], 
                // msg: {
                //     rgb_hex: "#FF0000"
                // }
            }
        },
        watch: {
            msg: function (newMsg) {
                if (newMsg?.payload?.availableProtocols) {
                    this.availableProtocols = newMsg?.payload?.availableProtocols;
                }
            }
        },
        methods: {
            handleProtocolToggle(selectedProtocol) {
                const isActive = this.selectedProtocols.includes(selectedProtocol);
                
                console.log(`Протокол ${selectedProtocol} тепер:`, isActive ? "УВІМКНЕНО" : "ВИМКНЕНО");
                
                this.send({
                    payload: {
                        protocol: selectedProtocol,
                        isActive: isActive,
                        allSelected: this.selectedProtocols
                    }
                });
            }
        },
    }
</script>